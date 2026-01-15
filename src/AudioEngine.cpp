#include "AudioEngine.h"

#include <alsa/asoundlib.h>
#include <cstring>
#include <vector>

#include "GlobalDefs.h"

namespace {

bool setupPcm(snd_pcm_t* pcm, int sampleRate, int channels, int periodFrames,
              int periods) {
  snd_pcm_hw_params_t* params = nullptr;
  snd_pcm_hw_params_alloca(&params);
  if (snd_pcm_hw_params_any(pcm, params) < 0) {
    return false;
  }

  if (snd_pcm_hw_params_set_access(pcm, params,
                                   SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
    return false;
  }
  if (snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE) < 0) {
    return false;
  }
  unsigned int rate = static_cast<unsigned int>(sampleRate);
  if (snd_pcm_hw_params_set_rate_near(pcm, params, &rate, nullptr) < 0) {
    return false;
  }
  if (snd_pcm_hw_params_set_channels(pcm, params, channels) < 0) {
    return false;
  }
  snd_pcm_uframes_t period = static_cast<snd_pcm_uframes_t>(periodFrames);
  if (snd_pcm_hw_params_set_period_size_near(pcm, params, &period, nullptr) <
      0) {
    return false;
  }
  snd_pcm_uframes_t bufferSize =
      static_cast<snd_pcm_uframes_t>(periodFrames * periods);
  if (snd_pcm_hw_params_set_buffer_size_near(pcm, params, &bufferSize) < 0) {
    return false;
  }

  if (snd_pcm_hw_params(pcm, params) < 0) {
    return false;
  }

  return true;
}

}  // namespace

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
  stop();
}

bool AudioEngine::init(const char* deviceName) {
  snd_pcm_t* playback = nullptr;
  snd_pcm_t* capture = nullptr;
  if (snd_pcm_open(&playback, deviceName, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    return false;
  }
  if (snd_pcm_open(&capture, deviceName, SND_PCM_STREAM_CAPTURE, 0) < 0) {
    snd_pcm_close(playback);
    return false;
  }

  if (!setupPcm(playback, defs::kSampleRate, defs::kChannels,
                defs::kPeriodFrames, defs::kPeriods)) {
    snd_pcm_close(playback);
    snd_pcm_close(capture);
    return false;
  }
  if (!setupPcm(capture, defs::kSampleRate, defs::kChannels,
                defs::kPeriodFrames, defs::kPeriods)) {
    snd_pcm_close(playback);
    snd_pcm_close(capture);
    return false;
  }

  playbackHandle_ = playback;
  captureHandle_ = capture;
  return true;
}

void AudioEngine::setCallback(AudioCallback cb, void* userData) {
  callback_ = cb;
  userData_ = userData;
}

bool AudioEngine::start() {
  if (running_.load()) {
    return true;
  }
  if (!playbackHandle_ || !captureHandle_) {
    return false;
  }
  running_.store(true);
  thread_ = std::thread(&AudioEngine::audioThread, this);
  return true;
}

void AudioEngine::stop() {
  if (!running_.load()) {
    return;
  }
  running_.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
  if (playbackHandle_) {
    snd_pcm_close(reinterpret_cast<snd_pcm_t*>(playbackHandle_));
    playbackHandle_ = nullptr;
  }
  if (captureHandle_) {
    snd_pcm_close(reinterpret_cast<snd_pcm_t*>(captureHandle_));
    captureHandle_ = nullptr;
  }
}

void AudioEngine::audioThread() {
  auto* playback = reinterpret_cast<snd_pcm_t*>(playbackHandle_);
  auto* capture = reinterpret_cast<snd_pcm_t*>(captureHandle_);

  const int frames = defs::kPeriodFrames;
  const int samples = frames * defs::kChannels;
  std::vector<int16_t> inInterleaved(samples, 0);
  std::vector<int16_t> outInterleaved(samples, 0);
  std::vector<float> inL(frames, 0.0f);
  std::vector<float> inR(frames, 0.0f);
  std::vector<float> outL(frames, 0.0f);
  std::vector<float> outR(frames, 0.0f);

  while (running_.load()) {
    snd_pcm_sframes_t readFrames =
        snd_pcm_readi(capture, inInterleaved.data(), frames);
    if (readFrames < 0) {
      snd_pcm_recover(capture, static_cast<int>(readFrames), 1);
      continue;
    }
    for (int i = 0; i < frames; ++i) {
      const int idx = i * 2;
      inL[i] = inInterleaved[idx] / 32768.0f;
      inR[i] = inInterleaved[idx + 1] / 32768.0f;
    }

    if (callback_) {
      callback_(inL.data(), inR.data(), outL.data(), outR.data(), frames,
                userData_);
    } else {
      std::memcpy(outL.data(), inL.data(), frames * sizeof(float));
      std::memcpy(outR.data(), inR.data(), frames * sizeof(float));
    }

    for (int i = 0; i < frames; ++i) {
      const int idx = i * 2;
      float l = outL[i];
      float r = outR[i];
      l = (l > 1.0f) ? 1.0f : (l < -1.0f ? -1.0f : l);
      r = (r > 1.0f) ? 1.0f : (r < -1.0f ? -1.0f : r);
      float lScaled = l * 32768.0f;
      float rScaled = r * 32768.0f;
      lScaled = (lScaled > 32767.0f) ? 32767.0f : (lScaled < -32768.0f ? -32768.0f : lScaled);
      rScaled = (rScaled > 32767.0f) ? 32767.0f : (rScaled < -32768.0f ? -32768.0f : rScaled);
      outInterleaved[idx] = static_cast<int16_t>(lScaled);
      outInterleaved[idx + 1] = static_cast<int16_t>(rScaled);
    }

    snd_pcm_sframes_t written =
        snd_pcm_writei(playback, outInterleaved.data(), frames);
    if (written < 0) {
      snd_pcm_recover(playback, static_cast<int>(written), 1);
    }
  }
}
