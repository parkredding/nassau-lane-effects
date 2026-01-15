#include <atomic>
#include <csignal>
#include <thread>
#include <vector>

#include "AudioEngine.h"
#include "GlobalDefs.h"
#include "HardwareManager.h"
#include "Effects/Saturation.h"
#include "Effects/DoubleTrack.h"
#include "Effects/Tremolo.h"
#include "Effects/Reverb.h"

struct ControlState {
  std::atomic<float> driveDb{4.0f};
  std::atomic<float> doubleMix{0.3f};
  std::atomic<float> lagMs{12.0f};
  std::atomic<float> wobble{0.2f};

  std::atomic<float> tremDepth{0.5f};
  std::atomic<float> tremRate{2.0f};
  std::atomic<float> revMix{0.2f};
  std::atomic<float> revDecay{0.6f};

  std::atomic<bool> bypass{false};
  std::atomic<defs::Bank> bank{defs::Bank::A};
};

struct AudioContext {
  Saturation sat;
  DoubleTrack dbl;
  Tremolo trem;
  Reverb rev;
  std::vector<float> tempL;
  std::vector<float> tempR;
  ControlState* controls = nullptr;
};

std::atomic<bool> gRunning{true};

void handleSignal(int) {
  gRunning.store(false);
}

float clampFloat(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

void audioCallback(const float* inL, const float* inR,
                   float* outL, float* outR, int nFrames,
                   void* userData) {
  auto* ctx = static_cast<AudioContext*>(userData);
  auto* state = ctx->controls;

  const bool bypass = state->bypass.load(std::memory_order_relaxed);
  const defs::Bank bank = state->bank.load(std::memory_order_relaxed);

  if (bypass) {
    for (int i = 0; i < nFrames; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  if (bank == defs::Bank::A) {
    ctx->sat.setDrive(state->driveDb.load(std::memory_order_relaxed));
    ctx->dbl.setMix(state->doubleMix.load(std::memory_order_relaxed));
    ctx->dbl.setLagMs(state->lagMs.load(std::memory_order_relaxed));
    ctx->dbl.setWobble(state->wobble.load(std::memory_order_relaxed));

    ctx->sat.process(inL, inR, ctx->tempL.data(), ctx->tempR.data(), nFrames);
    ctx->dbl.process(ctx->tempL.data(), ctx->tempR.data(), outL, outR,
                     nFrames);
  } else {
    ctx->trem.setDepth(state->tremDepth.load(std::memory_order_relaxed));
    ctx->trem.setRate(state->tremRate.load(std::memory_order_relaxed));
    ctx->rev.setMix(state->revMix.load(std::memory_order_relaxed));
    ctx->rev.setDecay(state->revDecay.load(std::memory_order_relaxed));

    ctx->trem.process(inL, inR, ctx->tempL.data(), ctx->tempR.data(), nFrames);
    ctx->rev.process(ctx->tempL.data(), ctx->tempR.data(), outL, outR,
                     nFrames);
  }
}

int main() {
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  AudioEngine engine;
  if (!engine.init("default")) {
    return 1;
  }

  HardwareManager hw;
  if (!hw.init()) {
    return 1;
  }
  hw.setLedColor(true, true, false);

  ControlState controls;
  AudioContext ctx;
  ctx.controls = &controls;
  ctx.sat.prepare(defs::kSampleRate, defs::kPeriodFrames);
  ctx.dbl.prepare(defs::kSampleRate, defs::kPeriodFrames);
  ctx.trem.prepare(defs::kSampleRate, defs::kPeriodFrames);
  ctx.rev.prepare(defs::kSampleRate, defs::kPeriodFrames);
  ctx.tempL.assign(defs::kPeriodFrames, 0.0f);
  ctx.tempR.assign(defs::kPeriodFrames, 0.0f);

  engine.setCallback(audioCallback, &ctx);
  if (!engine.start()) {
    return 1;
  }
  hw.setLedColor(false, true, true);

  while (gRunning.load()) {
    hw.poll();

    if (hw.consumeSwitchPress(HardwareManager::SwitchId::Bank)) {
      defs::Bank next = (controls.bank.load() == defs::Bank::A)
                            ? defs::Bank::B
                            : defs::Bank::A;
      controls.bank.store(next);
    }
    if (hw.consumeSwitchPress(HardwareManager::SwitchId::Bypass)) {
      bool next = !controls.bypass.load();
      controls.bypass.store(next);
    }

    if (hw.isSwitchHeld(HardwareManager::SwitchId::Shift)) {
      hw.setLedColor(false, true, false);
    } else {
      hw.setLedColor(false, true, true);
    }

    int d1 = hw.consumeEncoderDelta(0);
    int d2 = hw.consumeEncoderDelta(1);
    int d3 = hw.consumeEncoderDelta(2);

    const defs::Bank bank = controls.bank.load();
    if (bank == defs::Bank::A) {
      if (d1 != 0) {
        float v = controls.driveDb.load();
        v = clampFloat(v + d1 * 0.5f, defs::kMinDrive, defs::kMaxDrive);
        controls.driveDb.store(v);
      }
      if (d2 != 0) {
        float v = controls.doubleMix.load();
        v = clampFloat(v + d2 * 0.02f, defs::kMinMix, defs::kMaxMix);
        controls.doubleMix.store(v);
      }
      if (d3 != 0) {
        float v = controls.lagMs.load();
        float w = controls.wobble.load();
        v = clampFloat(v + d3 * 0.5f, defs::kMinLagMs, defs::kMaxLagMs);
        w = clampFloat(w + d3 * 0.02f, defs::kMinWobble, defs::kMaxWobble);
        controls.lagMs.store(v);
        controls.wobble.store(w);
      }
    } else {
      if (d1 != 0) {
        float v = controls.tremDepth.load();
        v = clampFloat(v + d1 * 0.02f, defs::kMinMix, defs::kMaxMix);
        controls.tremDepth.store(v);
      }
      if (d2 != 0) {
        float v = controls.revMix.load();
        v = clampFloat(v + d2 * 0.02f, defs::kMinMix, defs::kMaxMix);
        controls.revMix.store(v);
      }
      if (d3 != 0) {
        float rate = controls.tremRate.load();
        float decay = controls.revDecay.load();
        rate = clampFloat(rate + d3 * 0.2f, defs::kMinTremRateHz,
                          defs::kMaxTremRateHz);
        decay = clampFloat(decay + d3 * 0.02f, defs::kMinDecay, defs::kMaxDecay);
        controls.tremRate.store(rate);
        controls.revDecay.store(decay);
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  engine.stop();
  return 0;
}
