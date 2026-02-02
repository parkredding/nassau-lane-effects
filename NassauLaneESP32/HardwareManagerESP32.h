#pragma once

#include <Arduino.h>
#include "GlobalDefs.h"

class HardwareManagerESP32 {
 public:
  enum class SwitchId { Bank = 0, Bypass = 1, Shift = 2 };

  HardwareManagerESP32();
  bool init();
  void poll();  // Call this regularly (e.g., every 1ms)

  // Switch interface
  bool consumeSwitchPress(SwitchId id);
  bool isSwitchHeld(SwitchId id);

  // Encoder interface
  int consumeEncoderDelta(int encId);  // 0, 1, or 2

  // LED control
  void setLedColor(bool r, bool g, bool b);

 private:
  struct EncoderState {
    int clkPin;
    int dtPin;
    volatile int delta;
    volatile uint8_t lastState;
  };

  struct SwitchState {
    int pin;
    bool pressed;
    bool held;
    uint32_t pressTime;
    uint8_t lastReading;
  };

  EncoderState encoders_[3];
  SwitchState switches_[3];

  void updateEncoder(EncoderState& enc);
  void updateSwitch(SwitchState& sw);

  static const uint32_t kDebounceMs = 20;
  static const uint32_t kHoldThresholdMs = 500;
};
