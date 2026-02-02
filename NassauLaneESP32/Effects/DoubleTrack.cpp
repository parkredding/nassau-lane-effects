#include "DoubleTrack.h"
#include "../GlobalDefs.h"
#include <math.h>
#include <stdlib.h>

void DoubleTrack::prepare(int sampleRate, int blockSize) {
  (void)blockSize;
  sampleRate_ = sampleRate;

  // Allocate delay buffers
  const int maxDelaySamples = (int)(sampleRate_ * (defs::kMaxLagMs / 1000.0f)) + 4;

  if (delayL_) free(delayL_);
  if (delayR_) free(delayR_);

  delayL_ = (float*)calloc(maxDelaySamples, sizeof(float));
  delayR_ = (float*)calloc(maxDelaySamples, sizeof(float));
  delaySize_ = maxDelaySamples;
  writeIndex_ = 0;
}

void DoubleTrack::setMix(float mix) {
  mix_ = constrain(mix, defs::kMinMix, defs::kMaxMix);
}

void DoubleTrack::setLagMs(float ms) {
  lagMs_ = constrain(ms, defs::kMinLagMs, defs::kMaxLagMs);
}

void DoubleTrack::setWobble(float depth) {
  wobble_ = constrain(depth, defs::kMinWobble, defs::kMaxWobble);
}

void DoubleTrack::process(const float* inL, const float* inR,
                          float* outL, float* outR, int n) {
  if (!delayL_ || !delayR_) {
    for (int i = 0; i < n; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  const float lfoRate = 0.25f;
  const float kTwoPi = 6.283185307179586f;
  const float phaseInc = kTwoPi * lfoRate / sampleRate_;

  for (int i = 0; i < n; ++i) {
    delayL_[writeIndex_] = inL[i];
    delayR_[writeIndex_] = inR[i];

    const float wobbleSamples = (wobble_ * 0.5f * lagMs_) * (sampleRate_ / 1000.0f);
    const float mod = wobbleSamples * sin(phase_);
    const float baseDelay = lagMs_ * (sampleRate_ / 1000.0f) + mod;

    float readPos = (float)writeIndex_ - baseDelay;
    while (readPos < 0.0f) {
      readPos += delaySize_;
    }
    while (readPos >= (float)delaySize_) {
      readPos -= delaySize_;
    }

    int idx0 = (int)readPos;
    int idx1 = (idx0 + 1) % delaySize_;
    float frac = readPos - idx0;
    float dl = delayL_[idx0] + frac * (delayL_[idx1] - delayL_[idx0]);
    float dr = delayR_[idx0] + frac * (delayR_[idx1] - delayR_[idx0]);

    outL[i] = inL[i] * (1.0f - mix_) + dl * mix_;
    outR[i] = inR[i] * (1.0f - mix_) + dr * mix_;

    writeIndex_ = (writeIndex_ + 1) % delaySize_;
    phase_ += phaseInc;
    if (phase_ > kTwoPi) {
      phase_ -= kTwoPi;
    }
  }
}
