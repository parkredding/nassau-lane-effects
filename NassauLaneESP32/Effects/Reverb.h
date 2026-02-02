#pragma once

#include "EffectBase.h"

class Reverb : public EffectBase {
 public:
  void prepare(int sampleRate, int blockSize) override;
  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override;

  void setMix(float mix);
  void setDecay(float decay);

 private:
  float* comb1L_ = nullptr;
  float* comb1R_ = nullptr;
  float* comb2L_ = nullptr;
  float* comb2R_ = nullptr;
  float* ap1L_ = nullptr;
  float* ap1R_ = nullptr;

  int comb1Size_ = 0;
  int comb2Size_ = 0;
  int apSize_ = 0;

  int c1Index_ = 0;
  int c2Index_ = 0;
  int apIndex_ = 0;

  float mix_ = 0.2f;
  float decay_ = 0.6f;
};
