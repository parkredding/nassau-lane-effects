#pragma once

#include <algorithm>
#include <cmath>

namespace params {

// Bank A: Saturation Drive (Encoder 1)
// Range: Clean (1.0 gain) to Heavy Tape Distortion (20.0 gain / +26dB)
struct SaturationDrive {
  static constexpr float kMin = 1.0f;      // Clean
  static constexpr float kMax = 20.0f;      // Heavy Tape Distortion (+26dB)
  static constexpr float kDefault = 2.5f;   // Warm Breakup

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank A: Doubletrack Mix (Encoder 2)
// Range: 0.0 (Dry) to 1.0 (Wet)
struct DoubletrackMix {
  static constexpr float kMin = 0.0f;       // Dry
  static constexpr float kMax = 1.0f;       // Wet
  static constexpr float kDefault = 0.5f;    // Even blend

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank A: Lag Time (Encoder 3)
// Range: Tight Flanging (~0.5ms) to Slapback (~120ms)
struct LagTime {
  static constexpr float kMin = 0.5f;        // Tight Flanging
  static constexpr float kMax = 120.0f;      // Slapback
  static constexpr float kDefault = 15.0f;   // Thick Doubling

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank A: Wobble (Encoder 3 - secondary parameter)
// Range: 0.0 (No modulation) to 1.0 (Maximum modulation)
struct Wobble {
  static constexpr float kMin = 0.0f;
  static constexpr float kMax = 1.0f;
  static constexpr float kDefault = 0.2f;

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank B: Tremolo Intensity (Encoder 1)
// Range: 0.0 (Off) to 1.0 (Deep Chop)
struct TremoloIntensity {
  static constexpr float kMin = 0.0f;       // Off
  static constexpr float kMax = 1.0f;       // Deep Chop
  static constexpr float kDefault = 0.6f;   // Moderate depth

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank B: Reverb Mix (Encoder 2)
// Range: 0.0 (Dry) to 1.0 (Wet)
struct ReverbMix {
  static constexpr float kMin = 0.0f;       // Dry
  static constexpr float kMax = 1.0f;       // Wet
  static constexpr float kDefault = 0.3f;   // Subtle reverb

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank B: Tremolo Speed (Encoder 3 - primary parameter)
// Range: 0.5 Hz (Slow) to 12.0 Hz (Fast)
struct TremoloSpeed {
  static constexpr float kMin = 0.5f;        // Slow
  static constexpr float kMax = 12.0f;       // Fast
  static constexpr float kDefault = 4.0f;   // Moderate speed

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Bank B: Reverb Decay (Encoder 3 - secondary parameter)
// Range: 0.1s (Tiny Room) to 8.0s (Massive Hall)
struct ReverbDecay {
  static constexpr float kMin = 0.1f;       // Tiny Room
  static constexpr float kMax = 8.0f;        // Massive Hall
  static constexpr float kDefault = 2.0f;    // Medium hall

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

// Helper function to clamp any value to a parameter range
template <typename ParamType>
inline float clampToRange(float value) {
  return ParamType::clamp(value);
}

// Generic range structure for any parameter
template <float Min, float Max, float Default>
struct ParameterRange {
  static constexpr float kMin = Min;
  static constexpr float kMax = Max;
  static constexpr float kDefault = Default;

  static float clamp(float value) {
    return std::clamp(value, kMin, kMax);
  }
};

}  // namespace params
