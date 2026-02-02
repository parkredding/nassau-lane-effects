#pragma once

#include "EffectBase.h"

class Saturation : public EffectBase {
 public:
  void prepare(int sampleRate, int blockSize) override;
  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override;

  void setDrive(float driveDb);

 private:
  float driveDb_ = 0.0f;
  float driveLin_ = 1.0f;
  float smoothDrive_ = 1.0f;
};
