#include "HardwareManagerESP32.h"

HardwareManagerESP32::HardwareManagerESP32() {
  // Initialize encoder configurations
  encoders_[0] = {defs::kGpioEnc1Clk, defs::kGpioEnc1Dt, 0, 0};
  encoders_[1] = {defs::kGpioEnc2Clk, defs::kGpioEnc2Dt, 0, 0};
  encoders_[2] = {defs::kGpioEnc3Clk, defs::kGpioEnc3Dt, 0, 0};

  // Initialize switch configurations
  switches_[0] = {defs::kGpioSwBank, false, false, 0, HIGH};
  switches_[1] = {defs::kGpioSwBypass, false, false, 0, HIGH};
  switches_[2] = {defs::kGpioSwShift, false, false, 0, HIGH};
}

bool HardwareManagerESP32::init() {
  // Setup encoder pins
  for (int i = 0; i < 3; i++) {
    pinMode(encoders_[i].clkPin, INPUT_PULLUP);
    pinMode(encoders_[i].dtPin, INPUT_PULLUP);
    encoders_[i].lastState = (digitalRead(encoders_[i].clkPin) << 1) |
                              digitalRead(encoders_[i].dtPin);
  }

  // Setup switch pins (active LOW with internal pullup)
  for (int i = 0; i < 3; i++) {
    pinMode(switches_[i].pin, INPUT_PULLUP);
  }

  // Setup LED pins
  pinMode(defs::kGpioLedR, OUTPUT);
  pinMode(defs::kGpioLedG, OUTPUT);
  pinMode(defs::kGpioLedB, OUTPUT);
  setLedColor(false, false, false);

  Serial.println("Hardware manager initialized");
  return true;
}

void HardwareManagerESP32::poll() {
  // Update all encoders
  for (int i = 0; i < 3; i++) {
    updateEncoder(encoders_[i]);
  }

  // Update all switches
  for (int i = 0; i < 3; i++) {
    updateSwitch(switches_[i]);
  }
}

void HardwareManagerESP32::updateEncoder(EncoderState& enc) {
  uint8_t clk = digitalRead(enc.clkPin);
  uint8_t dt = digitalRead(enc.dtPin);
  uint8_t state = (clk << 1) | dt;

  // Gray code decoding for rotary encoder
  if (state != enc.lastState) {
    uint8_t combined = (enc.lastState << 2) | state;

    // Clockwise: 0b0001, 0b0111, 0b1110, 0b1000
    // Counter-clockwise: 0b0010, 0b1011, 0b1101, 0b0100
    if (combined == 0b0001 || combined == 0b0111 ||
        combined == 0b1110 || combined == 0b1000) {
      enc.delta++;
    } else if (combined == 0b0010 || combined == 0b1011 ||
               combined == 0b1101 || combined == 0b0100) {
      enc.delta--;
    }

    enc.lastState = state;
  }
}

void HardwareManagerESP32::updateSwitch(SwitchState& sw) {
  uint8_t reading = digitalRead(sw.pin);

  // Debounce logic
  if (reading != sw.lastReading) {
    sw.lastReading = reading;

    // Active LOW: pressed when reading is LOW
    if (reading == LOW && !sw.pressed && !sw.held) {
      sw.pressed = true;
      sw.pressTime = millis();
    } else if (reading == HIGH) {
      sw.pressed = false;
      sw.held = false;
    }
  }

  // Check for held state
  if (sw.pressed && !sw.held && (millis() - sw.pressTime > kHoldThresholdMs)) {
    sw.held = true;
  }
}

bool HardwareManagerESP32::consumeSwitchPress(SwitchId id) {
  int idx = (int)id;
  if (switches_[idx].pressed && !switches_[idx].held) {
    switches_[idx].pressed = false;
    return true;
  }
  return false;
}

bool HardwareManagerESP32::isSwitchHeld(SwitchId id) {
  int idx = (int)id;
  return switches_[idx].held;
}

int HardwareManagerESP32::consumeEncoderDelta(int encId) {
  if (encId < 0 || encId >= 3) return 0;
  int delta = encoders_[encId].delta;
  encoders_[encId].delta = 0;
  return delta;
}

void HardwareManagerESP32::setLedColor(bool r, bool g, bool b) {
  digitalWrite(defs::kGpioLedR, r ? HIGH : LOW);
  digitalWrite(defs::kGpioLedG, g ? HIGH : LOW);
  digitalWrite(defs::kGpioLedB, b ? HIGH : LOW);
}
