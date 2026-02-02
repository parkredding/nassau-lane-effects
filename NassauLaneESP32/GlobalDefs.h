#pragma once

// ============================================================================
// NASSAU LANE ESP32 - Global Definitions
// Heltec WiFi LoRa 32 V4 Configuration
// ============================================================================

namespace defs {

// Audio Configuration
constexpr int kSampleRate = 48000;
constexpr int kChannels = 2;
constexpr int kBufferSize = 128;  // Samples per channel
constexpr int kBitDepth = 16;

// I2S Pin Configuration (Heltec V4)
constexpr int kI2S_BCK = 41;      // Bit Clock
constexpr int kI2S_WS = 42;       // Word Select (LRCK)
constexpr int kI2S_DATA_OUT = 2;  // To DAC (PCM5102)
constexpr int kI2S_DATA_IN = 1;   // From ADC (PCM1808)

// Switch GPIO Pins
constexpr int kGpioSwBank = 45;    // Bank Toggle
constexpr int kGpioSwBypass = 46;  // Bypass
constexpr int kGpioSwShift = 3;    // Shift (Hold)

// Rotary Encoder GPIO Pins
constexpr int kGpioEnc1Clk = 4;
constexpr int kGpioEnc1Dt = 5;
constexpr int kGpioEnc2Clk = 6;
constexpr int kGpioEnc2Dt = 7;
constexpr int kGpioEnc3Clk = 15;
constexpr int kGpioEnc3Dt = 16;

// RGB LED Pins
constexpr int kGpioLedR = 47;
constexpr int kGpioLedG = 48;
constexpr int kGpioLedB = 21;

// OLED Display (Heltec V4 built-in)
constexpr int kOledSDA = 17;
constexpr int kOledSCL = 18;
constexpr int kOledRST = 21;
constexpr uint8_t kOledAddress = 0x3C;

// Parameter Ranges
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

// Bank Selection
enum class Bank : uint8_t {
  A = 0,  // Saturation + DoubleTrack
  B = 1,  // Tremolo + Reverb
};

}  // namespace defs
