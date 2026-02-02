// ============================================================================
// NASSAU LANE - ESP32 Port for Heltec WiFi LoRa 32 V4
// High-fidelity dual-engine guitar effects processor
// Copyright © 2026 Parker Redding. All Rights Reserved.
// ============================================================================

#include "GlobalDefs.h"
#include "AudioEngineESP32.h"
#include "HardwareManagerESP32.h"
#include "DisplayManager.h"
#include "Effects/Saturation.h"
#include "Effects/DoubleTrack.h"
#include "Effects/Tremolo.h"
#include "Effects/Reverb.h"

// ============================================================================
// Control State (shared between audio callback and main loop)
// ============================================================================

struct ControlState {
  volatile float driveDb = 4.0f;
  volatile float doubleMix = 0.3f;
  volatile float lagMs = 12.0f;
  volatile float wobble = 0.2f;

  volatile float tremDepth = 0.5f;
  volatile float tremRate = 2.0f;
  volatile float revMix = 0.2f;
  volatile float revDecay = 0.6f;

  volatile bool bypass = false;
  volatile defs::Bank bank = defs::Bank::A;
};

// ============================================================================
// Audio Processing Context
// ============================================================================

struct AudioContext {
  Saturation sat;
  DoubleTrack dbl;
  Tremolo trem;
  Reverb rev;

  float* tempL = nullptr;
  float* tempR = nullptr;

  ControlState* controls = nullptr;
};

// ============================================================================
// Global Objects
// ============================================================================

AudioEngineESP32 audioEngine;
HardwareManagerESP32 hardware;
DisplayManager display;

ControlState controls;
AudioContext audioContext;

uint32_t lastDisplayUpdate = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 50; // 20 Hz refresh

// ============================================================================
// Audio Callback (runs in audio thread context)
// ============================================================================

void audioCallback(const float* inL, const float* inR,
                   float* outL, float* outR, int nFrames,
                   void* userData) {
  AudioContext* ctx = (AudioContext*)userData;
  ControlState* state = ctx->controls;

  const bool bypass = state->bypass;
  const defs::Bank bank = state->bank;

  // Bypass mode: pass audio through
  if (bypass) {
    for (int i = 0; i < nFrames; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  // Bank A: Saturation → DoubleTrack
  if (bank == defs::Bank::A) {
    ctx->sat.setDrive(state->driveDb);
    ctx->dbl.setMix(state->doubleMix);
    ctx->dbl.setLagMs(state->lagMs);
    ctx->dbl.setWobble(state->wobble);

    ctx->sat.process(inL, inR, ctx->tempL, ctx->tempR, nFrames);
    ctx->dbl.process(ctx->tempL, ctx->tempR, outL, outR, nFrames);
  }
  // Bank B: Tremolo → Reverb
  else {
    ctx->trem.setDepth(state->tremDepth);
    ctx->trem.setRate(state->tremRate);
    ctx->rev.setMix(state->revMix);
    ctx->rev.setDecay(state->revDecay);

    ctx->trem.process(inL, inR, ctx->tempL, ctx->tempR, nFrames);
    ctx->rev.process(ctx->tempL, ctx->tempR, outL, outR, nFrames);
  }
}

// ============================================================================
// Utility Functions
// ============================================================================

float clampFloat(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

// ============================================================================
// Arduino Setup
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   NASSAU LANE - ESP32 Edition");
  Serial.println("   Heltec WiFi LoRa 32 V4");
  Serial.println("========================================");
  Serial.println();

  // Initialize display first for visual feedback
  if (!display.init()) {
    Serial.println("ERROR: Display initialization failed!");
    while (1) { delay(100); }
  }

  // Initialize hardware manager (GPIO, encoders, switches, LED)
  if (!hardware.init()) {
    Serial.println("ERROR: Hardware initialization failed!");
    while (1) { delay(100); }
  }
  hardware.setLedColor(true, true, false); // Yellow = initializing

  // Allocate temp buffers for audio processing
  audioContext.tempL = (float*)malloc(defs::kBufferSize * sizeof(float));
  audioContext.tempR = (float*)malloc(defs::kBufferSize * sizeof(float));

  if (!audioContext.tempL || !audioContext.tempR) {
    Serial.println("ERROR: Failed to allocate temp audio buffers!");
    while (1) { delay(100); }
  }

  // Initialize DSP effects
  Serial.println("Initializing DSP effects...");
  audioContext.sat.prepare(defs::kSampleRate, defs::kBufferSize);
  audioContext.dbl.prepare(defs::kSampleRate, defs::kBufferSize);
  audioContext.trem.prepare(defs::kSampleRate, defs::kBufferSize);
  audioContext.rev.prepare(defs::kSampleRate, defs::kBufferSize);
  audioContext.controls = &controls;

  // Initialize audio engine
  if (!audioEngine.init()) {
    Serial.println("ERROR: Audio engine initialization failed!");
    while (1) { delay(100); }
  }

  audioEngine.setCallback(audioCallback, &audioContext);

  if (!audioEngine.start()) {
    Serial.println("ERROR: Failed to start audio engine!");
    while (1) { delay(100); }
  }

  hardware.setLedColor(false, true, true); // Cyan = running

  Serial.println();
  Serial.println("========================================");
  Serial.println("   NASSAU LANE READY");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Controls:");
  Serial.println("  SW1: Bank Toggle (A/B)");
  Serial.println("  SW2: Bypass");
  Serial.println("  SW3: Shift (hold)");
  Serial.println();
}

// ============================================================================
// Arduino Main Loop
// ============================================================================

void loop() {
  // Process audio (call I2S read/write)
  audioEngine.process();

  // Poll hardware (switches and encoders)
  hardware.poll();

  // Handle switch inputs
  if (hardware.consumeSwitchPress(HardwareManagerESP32::SwitchId::Bank)) {
    defs::Bank next = (controls.bank == defs::Bank::A) ? defs::Bank::B : defs::Bank::A;
    controls.bank = next;
    Serial.print("Bank switched to: ");
    Serial.println((next == defs::Bank::A) ? "A (Saturation)" : "B (Ambience)");
  }

  if (hardware.consumeSwitchPress(HardwareManagerESP32::SwitchId::Bypass)) {
    controls.bypass = !controls.bypass;
    Serial.print("Bypass: ");
    Serial.println(controls.bypass ? "ON" : "OFF");
  }

  // Shift switch controls LED color
  if (hardware.isSwitchHeld(HardwareManagerESP32::SwitchId::Shift)) {
    hardware.setLedColor(false, true, false); // Green when shift held
  } else {
    hardware.setLedColor(false, true, true); // Cyan normal
  }

  // Read encoder deltas
  int d1 = hardware.consumeEncoderDelta(0);
  int d2 = hardware.consumeEncoderDelta(1);
  int d3 = hardware.consumeEncoderDelta(2);

  const defs::Bank bank = controls.bank;

  // Bank A parameter mapping
  if (bank == defs::Bank::A) {
    if (d1 != 0) {
      float v = controls.driveDb;
      v = clampFloat(v + d1 * 0.5f, defs::kMinDrive, defs::kMaxDrive);
      controls.driveDb = v;
    }
    if (d2 != 0) {
      float v = controls.doubleMix;
      v = clampFloat(v + d2 * 0.02f, defs::kMinMix, defs::kMaxMix);
      controls.doubleMix = v;
    }
    if (d3 != 0) {
      float v = controls.lagMs;
      float w = controls.wobble;
      v = clampFloat(v + d3 * 0.5f, defs::kMinLagMs, defs::kMaxLagMs);
      w = clampFloat(w + d3 * 0.02f, defs::kMinWobble, defs::kMaxWobble);
      controls.lagMs = v;
      controls.wobble = w;
    }
  }
  // Bank B parameter mapping
  else {
    if (d1 != 0) {
      float v = controls.tremDepth;
      v = clampFloat(v + d1 * 0.02f, defs::kMinMix, defs::kMaxMix);
      controls.tremDepth = v;
    }
    if (d2 != 0) {
      float v = controls.revMix;
      v = clampFloat(v + d2 * 0.02f, defs::kMinMix, defs::kMaxMix);
      controls.revMix = v;
    }
    if (d3 != 0) {
      float rate = controls.tremRate;
      float decay = controls.revDecay;
      rate = clampFloat(rate + d3 * 0.2f, defs::kMinTremRateHz, defs::kMaxTremRateHz);
      decay = clampFloat(decay + d3 * 0.02f, defs::kMinDecay, defs::kMaxDecay);
      controls.tremRate = rate;
      controls.revDecay = decay;
    }
  }

  // Update OLED display periodically
  uint32_t now = millis();
  if (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = now;

    if (bank == defs::Bank::A) {
      display.update(bank, controls.bypass,
                     controls.driveDb, controls.doubleMix, controls.lagMs);
    } else {
      display.update(bank, controls.bypass,
                     controls.tremDepth, controls.revMix, controls.tremRate);
    }
  }

  // Small delay to prevent watchdog timeout
  delayMicroseconds(100);
}
