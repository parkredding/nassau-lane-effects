#pragma once

#include <cstddef>
#include <cstdint>

namespace defs {

constexpr int kSampleRate = 48000;
constexpr int kChannels = 2;
constexpr int kPeriodFrames = 128;
constexpr int kPeriods = 2;

constexpr int kEncoderCount = 3;

constexpr int kGpioSwBank = 14;
constexpr int kGpioSwBypass = 15;
constexpr int kGpioSwShift = 16;

constexpr int kGpioLedR = 17;
constexpr int kGpioLedG = 27;
constexpr int kGpioLedB = 22;

constexpr int kGpioEnc1Clk = 2;
constexpr int kGpioEnc1Dt = 3;
constexpr int kGpioEnc2Clk = 4;
constexpr int kGpioEnc2Dt = 5;
constexpr int kGpioEnc3Clk = 6;
constexpr int kGpioEnc3Dt = 7;

constexpr float kMinDrive = 0.0f;
constexpr float kMaxDrive = 26.0f;
constexpr float kMinMix = 0.0f;
constexpr float kMaxMix = 1.0f;

constexpr float kMinLagMs = 4.0f;
constexpr float kMaxLagMs = 40.0f;
constexpr float kMinWobble = 0.0f;
constexpr float kMaxWobble = 1.0f;

constexpr float kMinTremRateHz = 0.2f;
constexpr float kMaxTremRateHz = 8.0f;
constexpr float kMinDecay = 0.1f;
constexpr float kMaxDecay = 8.0f;

enum class Bank : uint8_t {
  A = 0,
  B = 1,
};

}  // namespace defs
