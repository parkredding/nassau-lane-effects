#pragma once

#include <vector>

#include "Effects/EffectBase.h"

class Reverb : public EffectBase {
 public:
  void prepare(int sampleRate, int blockSize) override;
  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override;

  void setMix(float mix);
  void setDecay(float decay);

 private:
  std::vector<float> comb1L_;
  std::vector<float> comb1R_;
  std::vector<float> comb2L_;
  std::vector<float> comb2R_;
  std::vector<float> ap1L_;
  std::vector<float> ap1R_;

  int c1Index_ = 0;
  int c2Index_ = 0;
  int apIndex_ = 0;

  float mix_ = 0.2f;
  float decay_ = 0.6f;
};
