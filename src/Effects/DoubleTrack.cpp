#include "Effects/DoubleTrack.h"

#include <cmath>
#include <algorithm>

#include "GlobalDefs.h"

void DoubleTrack::prepare(int sampleRate, int blockSize) {
  (void)blockSize;
  sampleRate_ = sampleRate;
  const int maxDelaySamples =
      static_cast<int>(sampleRate_ * (defs::kMaxLagMs / 1000.0f)) + 4;
  delayL_.assign(maxDelaySamples, 0.0f);
  delayR_.assign(maxDelaySamples, 0.0f);
  writeIndex_ = 0;
}

void DoubleTrack::setMix(float mix) {
  mix_ = std::clamp(mix, defs::kMinMix, defs::kMaxMix);
}

void DoubleTrack::setLagMs(float ms) {
  lagMs_ = std::clamp(ms, defs::kMinLagMs, defs::kMaxLagMs);
}

void DoubleTrack::setWobble(float depth) {
  wobble_ = std::clamp(depth, defs::kMinWobble, defs::kMaxWobble);
}

void DoubleTrack::process(const float* inL, const float* inR,
                          float* outL, float* outR, int n) {
  if (delayL_.empty()) {
    for (int i = 0; i < n; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  const float lfoRate = 0.25f;
  constexpr float kTwoPi = 6.283185307179586f;
  const float phaseInc = kTwoPi * lfoRate / sampleRate_;
  const int size = static_cast<int>(delayL_.size());

  for (int i = 0; i < n; ++i) {
    delayL_[writeIndex_] = inL[i];
    delayR_[writeIndex_] = inR[i];

    const float wobbleSamples =
        (wobble_ * 0.5f * lagMs_) * (sampleRate_ / 1000.0f);
    const float mod = wobbleSamples * std::sin(phase_);
    const float baseDelay =
        lagMs_ * (sampleRate_ / 1000.0f) + mod;
    float readPos = static_cast<float>(writeIndex_) - baseDelay;
    while (readPos < 0.0f) {
      readPos += size;
    }
    while (readPos >= static_cast<float>(size)) {
      readPos -= size;
    }
    int idx0 = static_cast<int>(readPos);
    int idx1 = (idx0 + 1) % size;
    float frac = readPos - idx0;
    float dl = delayL_[idx0] + frac * (delayL_[idx1] - delayL_[idx0]);
    float dr = delayR_[idx0] + frac * (delayR_[idx1] - delayR_[idx0]);

    outL[i] = inL[i] * (1.0f - mix_) + dl * mix_;
    outR[i] = inR[i] * (1.0f - mix_) + dr * mix_;

    writeIndex_ = (writeIndex_ + 1) % size;
    phase_ += phaseInc;
    if (phase_ > kTwoPi) {
      phase_ -= kTwoPi;
    }
  }
}
