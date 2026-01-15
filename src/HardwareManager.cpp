#include "HardwareManager.h"

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstdio>

#include "GlobalDefs.h"

namespace {

constexpr int kDebounceSamples = 3;

bool exportGpio(int pin) {
  char path[64];
  std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);
  if (access(path, F_OK) == 0) {
    return true;
  }
  int fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0) {
    return false;
  }
  char buf[16];
  int len = std::snprintf(buf, sizeof(buf), "%d", pin);
  bool ok = write(fd, buf, len) == len;
  close(fd);
  return ok;
}

bool setDirectionIn(int pin) {
  char path[64];
  std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return false;
  }
  bool ok = write(fd, "in", 2) == 2;
  close(fd);
  return ok;
}

bool setDirectionOut(int pin) {
  char path[64];
  std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return false;
  }
  bool ok = write(fd, "out", 3) == 3;
  close(fd);
  return ok;
}

bool readGpioValue(int pin) {
  char path[64];
  std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return false;
  }
  char value = '0';
  read(fd, &value, 1);
  close(fd);
  return value == '1';
}

bool writeGpioValue(int pin, bool value) {
  char path[64];
  std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
  int fd = open(path, O_WRONLY);
  if (fd < 0) {
    return false;
  }
  const char v = value ? '1' : '0';
  bool ok = write(fd, &v, 1) == 1;
  close(fd);
  return ok;
}

}  // namespace

bool HardwareManager::init() {
  const std::array<int, 9> inputPins = {
      defs::kGpioSwBank, defs::kGpioSwBypass, defs::kGpioSwShift,
      defs::kGpioEnc1Clk, defs::kGpioEnc1Dt, defs::kGpioEnc2Clk,
      defs::kGpioEnc2Dt, defs::kGpioEnc3Clk, defs::kGpioEnc3Dt};
  const std::array<int, 3> outputPins = {
      defs::kGpioLedR, defs::kGpioLedG, defs::kGpioLedB};

  for (int pin : inputPins) {
    if (!exportGpio(pin)) {
      return false;
    }
    if (!setDirectionIn(pin)) {
      return false;
    }
  }

  for (int pin : outputPins) {
    if (!exportGpio(pin)) {
      return false;
    }
    if (!setDirectionOut(pin)) {
      return false;
    }
    if (!writeGpioValue(pin, false)) {
      return false;
    }
  }

  poll();
  return true;
}

void HardwareManager::poll() {
  const bool bankRaw = !readGpioValue(defs::kGpioSwBank);
  const bool bypassRaw = !readGpioValue(defs::kGpioSwBypass);
  const bool shiftRaw = !readGpioValue(defs::kGpioSwShift);

  const std::array<bool, 3> rawStates = {bankRaw, bypassRaw, shiftRaw};
  for (size_t i = 0; i < rawStates.size(); ++i) {
    auto& sw = switches_[i];
    if (rawStates[i] != sw.stableState) {
      sw.debounceCount++;
      if (sw.debounceCount >= kDebounceSamples) {
        sw.stableState = rawStates[i];
        sw.debounceCount = 0;
        if (sw.stableState) {
          sw.pressedEdge = true;
        } else {
          sw.pressedEdge = false;
        }
      }
    } else {
      sw.debounceCount = 0;
    }
  }

  const std::array<std::array<int, 2>, 3> encPins = {{
      {defs::kGpioEnc1Clk, defs::kGpioEnc1Dt},
      {defs::kGpioEnc2Clk, defs::kGpioEnc2Dt},
      {defs::kGpioEnc3Clk, defs::kGpioEnc3Dt},
  }};

  static constexpr int8_t kEncoderTable[16] = {
      0, -1, 1, 0, 1, 0, 0, -1,
      -1, 0, 0, 1, 0, 1, -1, 0,
  };

  for (size_t i = 0; i < encPins.size(); ++i) {
    int clk = readGpioValue(encPins[i][0]) ? 1 : 0;
    int dt = readGpioValue(encPins[i][1]) ? 1 : 0;
    int state = (clk << 1) | dt;
    int transition = (encoders_[i].lastState << 2) | state;
    encoders_[i].delta += kEncoderTable[transition];
    encoders_[i].lastState = state;
  }
}

int HardwareManager::consumeEncoderDelta(int index) {
  if (index < 0 || index >= static_cast<int>(encoders_.size())) {
    return 0;
  }
  int d = encoders_[index].delta;
  encoders_[index].delta = 0;
  return d;
}

bool HardwareManager::consumeSwitchPress(SwitchId id) {
  const int idx = static_cast<int>(id);
  if (idx < 0 || idx >= static_cast<int>(switches_.size())) {
    return false;
  }
  bool pressed = switches_[idx].pressedEdge;
  switches_[idx].pressedEdge = false;
  return pressed;
}

bool HardwareManager::isSwitchHeld(SwitchId id) const {
  const int idx = static_cast<int>(id);
  if (idx < 0 || idx >= static_cast<int>(switches_.size())) {
    return false;
  }
  return switches_[idx].stableState;
}

void HardwareManager::setLedColor(bool red, bool green, bool blue) {
  if (red == ledR_ && green == ledG_ && blue == ledB_) {
    return;
  }
  writeGpioValue(defs::kGpioLedR, red);
  writeGpioValue(defs::kGpioLedG, green);
  writeGpioValue(defs::kGpioLedB, blue);
  ledR_ = red;
  ledG_ = green;
  ledB_ = blue;
}
