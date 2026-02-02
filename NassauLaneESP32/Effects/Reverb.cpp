#include "Reverb.h"
#include "../GlobalDefs.h"
#include <stdlib.h>

void Reverb::prepare(int sampleRate, int blockSize) {
  (void)blockSize;

  comb1Size_ = (int)(sampleRate * 0.0297f);
  comb2Size_ = (int)(sampleRate * 0.0371f);
  apSize_ = (int)(sampleRate * 0.005f);

  if (comb1L_) free(comb1L_);
  if (comb1R_) free(comb1R_);
  if (comb2L_) free(comb2L_);
  if (comb2R_) free(comb2R_);
  if (ap1L_) free(ap1L_);
  if (ap1R_) free(ap1R_);

  comb1L_ = (float*)calloc(comb1Size_, sizeof(float));
  comb1R_ = (float*)calloc(comb1Size_, sizeof(float));
  comb2L_ = (float*)calloc(comb2Size_, sizeof(float));
  comb2R_ = (float*)calloc(comb2Size_, sizeof(float));
  ap1L_ = (float*)calloc(apSize_, sizeof(float));
  ap1R_ = (float*)calloc(apSize_, sizeof(float));

  c1Index_ = 0;
  c2Index_ = 0;
  apIndex_ = 0;
}

void Reverb::setMix(float mix) {
  mix_ = constrain(mix, defs::kMinMix, defs::kMaxMix);
}

void Reverb::setDecay(float decay) {
  decay_ = constrain(decay, defs::kMinDecay, defs::kMaxDecay);
}

void Reverb::process(const float* inL, const float* inR,
                     float* outL, float* outR, int n) {
  if (!comb1L_ || !comb1R_) {
    for (int i = 0; i < n; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  for (int i = 0; i < n; ++i) {
    float inSampleL = inL[i];
    float inSampleR = inR[i];

    float c1OutL = comb1L_[c1Index_];
    float c1OutR = comb1R_[c1Index_];
    comb1L_[c1Index_] = inSampleL + c1OutL * decay_;
    comb1R_[c1Index_] = inSampleR + c1OutR * decay_;
    c1Index_ = (c1Index_ + 1) % comb1Size_;

    float c2OutL = comb2L_[c2Index_];
    float c2OutR = comb2R_[c2Index_];
    comb2L_[c2Index_] = inSampleL + c2OutL * decay_;
    comb2R_[c2Index_] = inSampleR + c2OutR * decay_;
    c2Index_ = (c2Index_ + 1) % comb2Size_;

    float combL = 0.5f * (c1OutL + c2OutL);
    float combR = 0.5f * (c1OutR + c2OutR);

    float apInL = combL;
    float apInR = combR;
    float apOutL = ap1L_[apIndex_];
    float apOutR = ap1R_[apIndex_];
    ap1L_[apIndex_] = apInL + apOutL * 0.5f;
    ap1R_[apIndex_] = apInR + apOutR * 0.5f;
    float diffL = -apInL + apOutL;
    float diffR = -apInR + apOutR;
    apIndex_ = (apIndex_ + 1) % apSize_;

    outL[i] = inSampleL * (1.0f - mix_) + diffL * mix_;
    outR[i] = inSampleR * (1.0f - mix_) + diffR * mix_;
  }
}
