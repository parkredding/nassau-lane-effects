#include "Effects/Reverb.h"

#include <algorithm>

#include "GlobalDefs.h"

void Reverb::prepare(int sampleRate, int blockSize) {
  (void)blockSize;
  const int comb1 = static_cast<int>(sampleRate * 0.0297f);
  const int comb2 = static_cast<int>(sampleRate * 0.0371f);
  const int allpass = static_cast<int>(sampleRate * 0.005f);

  comb1L_.assign(comb1, 0.0f);
  comb1R_.assign(comb1, 0.0f);
  comb2L_.assign(comb2, 0.0f);
  comb2R_.assign(comb2, 0.0f);
  ap1L_.assign(allpass, 0.0f);
  ap1R_.assign(allpass, 0.0f);

  c1Index_ = 0;
  c2Index_ = 0;
  apIndex_ = 0;
}

void Reverb::setMix(float mix) {
  mix_ = std::clamp(mix, defs::kMinMix, defs::kMaxMix);
}

void Reverb::setDecay(float decay) {
  decay_ = std::clamp(decay, defs::kMinDecay, defs::kMaxDecay);
}

void Reverb::process(const float* inL, const float* inR,
                     float* outL, float* outR, int n) {
  if (comb1L_.empty()) {
    for (int i = 0; i < n; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  const int c1Size = static_cast<int>(comb1L_.size());
  const int c2Size = static_cast<int>(comb2L_.size());
  const int apSize = static_cast<int>(ap1L_.size());

  for (int i = 0; i < n; ++i) {
    float inSampleL = inL[i];
    float inSampleR = inR[i];

    float c1OutL = comb1L_[c1Index_];
    float c1OutR = comb1R_[c1Index_];
    comb1L_[c1Index_] = inSampleL + c1OutL * decay_;
    comb1R_[c1Index_] = inSampleR + c1OutR * decay_;
    c1Index_ = (c1Index_ + 1) % c1Size;

    float c2OutL = comb2L_[c2Index_];
    float c2OutR = comb2R_[c2Index_];
    comb2L_[c2Index_] = inSampleL + c2OutL * decay_;
    comb2R_[c2Index_] = inSampleR + c2OutR * decay_;
    c2Index_ = (c2Index_ + 1) % c2Size;

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
    apIndex_ = (apIndex_ + 1) % apSize;

    outL[i] = inSampleL * (1.0f - mix_) + diffL * mix_;
    outR[i] = inSampleR * (1.0f - mix_) + diffR * mix_;
  }
}
