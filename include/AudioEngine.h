#pragma once

#include <atomic>
#include <thread>

class AudioEngine {
 public:
  using AudioCallback = void (*)(const float* inL, const float* inR,
                                 float* outL, float* outR, int nFrames,
                                 void* userData);

  AudioEngine();
  ~AudioEngine();

  bool init(const char* deviceName);
  void setCallback(AudioCallback cb, void* userData);
  bool start();
  void stop();

 private:
  void audioThread();

  void* playbackHandle_ = nullptr;
  void* captureHandle_ = nullptr;

  AudioCallback callback_ = nullptr;
  void* userData_ = nullptr;

  std::thread thread_;
  std::atomic<bool> running_{false};
};
