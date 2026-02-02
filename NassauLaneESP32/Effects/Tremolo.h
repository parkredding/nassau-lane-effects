#pragma once

#include "EffectBase.h"

class Tremolo : public EffectBase {
 public:
  void prepare(int sampleRate, int blockSize) override;
  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override;

  void setDepth(float depth);
  void setRate(float hz);

 private:
  int sampleRate_ = 48000;
  float depth_ = 0.5f;
  float rate_ = 2.0f;
  float phase_ = 0.0f;
};
