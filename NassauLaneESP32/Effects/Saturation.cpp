#include "Saturation.h"
#include <math.h>

void Saturation::prepare(int sampleRate, int blockSize) {
  (void)sampleRate;
  (void)blockSize;
}

void Saturation::setDrive(float driveDb) {
  driveDb_ = driveDb;
  driveLin_ = pow(10.0f, driveDb_ / 20.0f);
}

void Saturation::process(const float* inL, const float* inR,
                         float* outL, float* outR, int n) {
  const float smoothCoeff = 0.02f;
  smoothDrive_ = smoothValue(smoothDrive_, driveLin_, smoothCoeff);
  for (int i = 0; i < n; ++i) {
    float l = inL[i] * smoothDrive_;
    float r = inR[i] * smoothDrive_;
    outL[i] = tanh(l);
    outR[i] = tanh(r);
  }
}
