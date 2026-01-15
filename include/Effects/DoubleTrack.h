#pragma once

#include <vector>

#include "Effects/EffectBase.h"

class DoubleTrack : public EffectBase {
 public:
  void prepare(int sampleRate, int blockSize) override;
  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override;

  void setMix(float mix);
  void setLagMs(float ms);
  void setWobble(float depth);

 private:
  std::vector<float> delayL_;
  std::vector<float> delayR_;
  int writeIndex_ = 0;
  int sampleRate_ = 48000;

  float mix_ = 0.3f;
  float lagMs_ = 12.0f;
  float wobble_ = 0.2f;
  float phase_ = 0.0f;
};
