#include "Tremolo.h"
#include "../GlobalDefs.h"
#include <math.h>

void Tremolo::prepare(int sampleRate, int blockSize) {
  (void)blockSize;
  sampleRate_ = sampleRate;
}

void Tremolo::setDepth(float depth) {
  depth_ = constrain(depth, defs::kMinMix, defs::kMaxMix);
}

void Tremolo::setRate(float hz) {
  rate_ = constrain(hz, defs::kMinTremRateHz, defs::kMaxTremRateHz);
}

void Tremolo::process(const float* inL, const float* inR,
                      float* outL, float* outR, int n) {
  const float kTwoPi = 6.283185307179586f;
  const float phaseInc = kTwoPi * rate_ / sampleRate_;
  for (int i = 0; i < n; ++i) {
    float mod = 0.5f * (1.0f + sin(phase_));
    float gain = (1.0f - depth_) + depth_ * mod;
    outL[i] = inL[i] * gain;
    outR[i] = inR[i] * gain;
    phase_ += phaseInc;
    if (phase_ > kTwoPi) {
      phase_ -= kTwoPi;
    }
  }
}
