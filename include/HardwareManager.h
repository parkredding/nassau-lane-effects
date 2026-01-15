#pragma once

#include <array>

class HardwareManager {
 public:
  enum class SwitchId {
    Bank = 0,
    Bypass = 1,
    Shift = 2,
    Count = 3,
  };

  bool init();
  void poll();

  int consumeEncoderDelta(int index);
  bool consumeSwitchPress(SwitchId id);
  bool isSwitchHeld(SwitchId id) const;

  void setLedColor(bool red, bool green, bool blue);

 private:
  struct SwitchState {
    bool stableState = false;
    int debounceCount = 0;
    bool pressedEdge = false;
  };

  struct EncoderState {
    int lastState = 0;
    int delta = 0;
  };

  std::array<SwitchState, 3> switches_{};
  std::array<EncoderState, 3> encoders_{};
  bool ledR_ = false;
  bool ledG_ = false;
  bool ledB_ = false;
};
