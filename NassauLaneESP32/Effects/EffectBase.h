#pragma once

class EffectBase {
 public:
  virtual ~EffectBase() = default;
  virtual void prepare(int sampleRate, int blockSize) = 0;
  virtual void process(const float* inL, const float* inR,
                       float* outL, float* outR, int n) = 0;

 protected:
  static float smoothValue(float current, float target, float coeff) {
    return current + coeff * (target - current);
  }
};
