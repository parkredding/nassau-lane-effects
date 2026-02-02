#pragma once

#include <driver/i2s.h>
#include "GlobalDefs.h"

class AudioEngineESP32 {
 public:
  typedef void (*AudioCallback)(const float* inL, const float* inR,
                                float* outL, float* outR, int nFrames,
                                void* userData);

  AudioEngineESP32();
  ~AudioEngineESP32();

  bool init();
  void setCallback(AudioCallback cb, void* userData);
  bool start();
  void stop();
  void process();  // Call this in loop()

 private:
  void convertInt16ToFloat(const int16_t* src, float* dstL, float* dstR, int frames);
  void convertFloatToInt16(const float* srcL, const float* srcR, int16_t* dst, int frames);

  AudioCallback callback_ = nullptr;
  void* userData_ = nullptr;

  int16_t* i2sInputBuffer_ = nullptr;
  int16_t* i2sOutputBuffer_ = nullptr;
  float* floatInputL_ = nullptr;
  float* floatInputR_ = nullptr;
  float* floatOutputL_ = nullptr;
  float* floatOutputR_ = nullptr;

  bool running_ = false;
};
