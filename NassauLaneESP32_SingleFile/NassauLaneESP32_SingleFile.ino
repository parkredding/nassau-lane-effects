/*
 * ============================================================================
 * NASSAU LANE - Guitar Effects Processor
 * Heltec WiFi LoRa 32 V4 Edition (ESP32-S3)
 * ============================================================================
 *
 * A high-fidelity dual-engine digital effects processor with OLED display
 *
 * Features:
 * - Bank A: Tape Saturation + Double Tracking
 * - Bank B: Harmonic Tremolo + Reverb
 * - 48kHz sample rate, 128-sample buffers
 * - Real-time OLED parameter display
 * - 3 rotary encoders for control
 * - 3 footswitches (Bank, Bypass, Shift)
 *
 * Hardware Requirements:
 * - Heltec WiFi LoRa 32 V4
 * - PCM5102A DAC (I2S output)
 * - PCM1808 ADC (I2S input)
 * - 3x Rotary Encoders
 * - 3x Momentary Footswitches
 * - 1x RGB LED (common cathode)
 *
 * Copyright © 2026 Parker Redding. All Rights Reserved.
 * ============================================================================
 */

// ============================================================================
// INCLUDES
// ============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>

// ============================================================================
// CONFIGURATION - HELTEC V4 SPECIFIC
// ============================================================================

// Audio Configuration
#define SAMPLE_RATE         48000
#define BUFFER_SIZE         128
#define CHANNELS            2

// Heltec V4 I2S Pins (optimized for V4 layout)
#define I2S_BCK_PIN         41    // Bit Clock
#define I2S_WS_PIN          42    // Word Select (LRCK)
#define I2S_DATA_OUT_PIN    2     // To PCM5102A DAC
#define I2S_DATA_IN_PIN     1     // From PCM1808 ADC

// Heltec V4 OLED (built-in)
#define OLED_SDA            17
#define OLED_SCL            18
#define OLED_RST            21
#define OLED_ADDR           0x3C
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// Control GPIO Pins
#define SW_BANK_PIN         45    // Bank toggle switch
#define SW_BYPASS_PIN       46    // Bypass switch
#define SW_SHIFT_PIN        3     // Shift switch (hold)

// Rotary Encoder Pins
#define ENC1_CLK_PIN        4
#define ENC1_DT_PIN         5
#define ENC2_CLK_PIN        6
#define ENC2_DT_PIN         7
#define ENC3_CLK_PIN        15
#define ENC3_DT_PIN         16

// RGB LED Pins
#define LED_R_PIN           47
#define LED_G_PIN           48
#define LED_B_PIN           35

// Parameter Ranges
#define MIN_DRIVE           0.0f
#define MAX_DRIVE           26.0f
#define MIN_MIX             0.0f
#define MAX_MIX             1.0f
#define MIN_LAG_MS          4.0f
#define MAX_LAG_MS          40.0f
#define MIN_WOBBLE          0.0f
#define MAX_WOBBLE          1.0f
#define MIN_TREM_RATE_HZ    0.2f
#define MAX_TREM_RATE_HZ    8.0f
#define MIN_DECAY           0.1f
#define MAX_DECAY           8.0f

// ============================================================================
// GLOBAL TYPES
// ============================================================================

enum Bank { BANK_A = 0, BANK_B = 1 };

// ============================================================================
// CONTROL STATE (shared between audio callback and main loop)
// ============================================================================

struct ControlState {
  // Bank A parameters
  volatile float driveDb = 4.0f;
  volatile float doubleMix = 0.3f;
  volatile float lagMs = 12.0f;
  volatile float wobble = 0.2f;

  // Bank B parameters
  volatile float tremDepth = 0.5f;
  volatile float tremRate = 2.0f;
  volatile float revMix = 0.2f;
  volatile float revDecay = 0.6f;

  // Global state
  volatile bool bypass = false;
  volatile Bank bank = BANK_A;
};

// ============================================================================
// ENCODER STATE
// ============================================================================

struct EncoderState {
  int clkPin;
  int dtPin;
  volatile int delta;
  volatile uint8_t lastState;
};

// ============================================================================
// SWITCH STATE
// ============================================================================

struct SwitchState {
  int pin;
  bool pressed;
  bool held;
  uint32_t pressTime;
  uint8_t lastReading;
};

// ============================================================================
// DSP EFFECT BASE CLASS
// ============================================================================

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

// ============================================================================
// SATURATION EFFECT
// ============================================================================

class Saturation : public EffectBase {
public:
  void prepare(int sampleRate, int blockSize) override {
    (void)sampleRate;
    (void)blockSize;
  }

  void setDrive(float driveDb) {
    driveDb_ = driveDb;
    driveLin_ = pow(10.0f, driveDb_ / 20.0f);
  }

  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override {
    const float smoothCoeff = 0.02f;
    smoothDrive_ = smoothValue(smoothDrive_, driveLin_, smoothCoeff);
    for (int i = 0; i < n; ++i) {
      float l = inL[i] * smoothDrive_;
      float r = inR[i] * smoothDrive_;
      outL[i] = tanh(l);
      outR[i] = tanh(r);
    }
  }

private:
  float driveDb_ = 0.0f;
  float driveLin_ = 1.0f;
  float smoothDrive_ = 1.0f;
};

// ============================================================================
// DOUBLE TRACK EFFECT
// ============================================================================

class DoubleTrack : public EffectBase {
public:
  ~DoubleTrack() {
    if (delayL_) free(delayL_);
    if (delayR_) free(delayR_);
  }

  void prepare(int sampleRate, int blockSize) override {
    (void)blockSize;
    sampleRate_ = sampleRate;

    const int maxDelaySamples = (int)(sampleRate_ * (MAX_LAG_MS / 1000.0f)) + 4;

    if (delayL_) free(delayL_);
    if (delayR_) free(delayR_);

    delayL_ = (float*)ps_calloc(maxDelaySamples, sizeof(float));
    delayR_ = (float*)ps_calloc(maxDelaySamples, sizeof(float));
    delaySize_ = maxDelaySamples;
    writeIndex_ = 0;
  }

  void setMix(float mix) {
    mix_ = constrain(mix, MIN_MIX, MAX_MIX);
  }

  void setLagMs(float ms) {
    lagMs_ = constrain(ms, MIN_LAG_MS, MAX_LAG_MS);
  }

  void setWobble(float depth) {
    wobble_ = constrain(depth, MIN_WOBBLE, MAX_WOBBLE);
  }

  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override {
    if (!delayL_ || !delayR_) {
      for (int i = 0; i < n; ++i) {
        outL[i] = inL[i];
        outR[i] = inR[i];
      }
      return;
    }

    const float lfoRate = 0.25f;
    const float kTwoPi = 6.283185307179586f;
    const float phaseInc = kTwoPi * lfoRate / sampleRate_;

    for (int i = 0; i < n; ++i) {
      delayL_[writeIndex_] = inL[i];
      delayR_[writeIndex_] = inR[i];

      const float wobbleSamples = (wobble_ * 0.5f * lagMs_) * (sampleRate_ / 1000.0f);
      const float mod = wobbleSamples * sin(phase_);
      const float baseDelay = lagMs_ * (sampleRate_ / 1000.0f) + mod;

      float readPos = (float)writeIndex_ - baseDelay;
      while (readPos < 0.0f) {
        readPos += delaySize_;
      }
      while (readPos >= (float)delaySize_) {
        readPos -= delaySize_;
      }

      int idx0 = (int)readPos;
      int idx1 = (idx0 + 1) % delaySize_;
      float frac = readPos - idx0;
      float dl = delayL_[idx0] + frac * (delayL_[idx1] - delayL_[idx0]);
      float dr = delayR_[idx0] + frac * (delayR_[idx1] - delayR_[idx0]);

      outL[i] = inL[i] * (1.0f - mix_) + dl * mix_;
      outR[i] = inR[i] * (1.0f - mix_) + dr * mix_;

      writeIndex_ = (writeIndex_ + 1) % delaySize_;
      phase_ += phaseInc;
      if (phase_ > kTwoPi) {
        phase_ -= kTwoPi;
      }
    }
  }

private:
  float* delayL_ = nullptr;
  float* delayR_ = nullptr;
  int delaySize_ = 0;
  int writeIndex_ = 0;
  int sampleRate_ = 48000;
  float mix_ = 0.3f;
  float lagMs_ = 12.0f;
  float wobble_ = 0.2f;
  float phase_ = 0.0f;
};

// ============================================================================
// TREMOLO EFFECT
// ============================================================================

class Tremolo : public EffectBase {
public:
  void prepare(int sampleRate, int blockSize) override {
    (void)blockSize;
    sampleRate_ = sampleRate;
  }

  void setDepth(float depth) {
    depth_ = constrain(depth, MIN_MIX, MAX_MIX);
  }

  void setRate(float hz) {
    rate_ = constrain(hz, MIN_TREM_RATE_HZ, MAX_TREM_RATE_HZ);
  }

  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override {
    const float kTwoPi = 6.283185307179586f;
    const float phaseInc = kTwoPi * rate_ / sampleRate_;
    for (int i = 0; i < n; ++i) {
      float mod = 0.5f * (1.0f + sin(phase_));
      float gain = (1.0f - depth_) + depth_ * mod;
      outL[i] = inL[i] * gain;
      outR[i] = inR[i] * gain;
      phase_ += phaseInc;
      if (phase_ > kTwoPi) {
        phase_ -= kTwoPi;
      }
    }
  }

private:
  int sampleRate_ = 48000;
  float depth_ = 0.5f;
  float rate_ = 2.0f;
  float phase_ = 0.0f;
};

// ============================================================================
// REVERB EFFECT
// ============================================================================

class Reverb : public EffectBase {
public:
  ~Reverb() {
    if (comb1L_) free(comb1L_);
    if (comb1R_) free(comb1R_);
    if (comb2L_) free(comb2L_);
    if (comb2R_) free(comb2R_);
    if (ap1L_) free(ap1L_);
    if (ap1R_) free(ap1R_);
  }

  void prepare(int sampleRate, int blockSize) override {
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

    comb1L_ = (float*)ps_calloc(comb1Size_, sizeof(float));
    comb1R_ = (float*)ps_calloc(comb1Size_, sizeof(float));
    comb2L_ = (float*)ps_calloc(comb2Size_, sizeof(float));
    comb2R_ = (float*)ps_calloc(comb2Size_, sizeof(float));
    ap1L_ = (float*)ps_calloc(apSize_, sizeof(float));
    ap1R_ = (float*)ps_calloc(apSize_, sizeof(float));

    c1Index_ = 0;
    c2Index_ = 0;
    apIndex_ = 0;
  }

  void setMix(float mix) {
    mix_ = constrain(mix, MIN_MIX, MAX_MIX);
  }

  void setDecay(float decay) {
    decay_ = constrain(decay, MIN_DECAY, MAX_DECAY);
  }

  void process(const float* inL, const float* inR,
               float* outL, float* outR, int n) override {
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

private:
  float* comb1L_ = nullptr;
  float* comb1R_ = nullptr;
  float* comb2L_ = nullptr;
  float* comb2R_ = nullptr;
  float* ap1L_ = nullptr;
  float* ap1R_ = nullptr;

  int comb1Size_ = 0;
  int comb2Size_ = 0;
  int apSize_ = 0;

  int c1Index_ = 0;
  int c2Index_ = 0;
  int apIndex_ = 0;

  float mix_ = 0.2f;
  float decay_ = 0.6f;
};

// ============================================================================
// AUDIO CONTEXT
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
// GLOBAL VARIABLES
// ============================================================================

// Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Audio buffers
int16_t* i2sInputBuffer = nullptr;
int16_t* i2sOutputBuffer = nullptr;
float* floatInputL = nullptr;
float* floatInputR = nullptr;
float* floatOutputL = nullptr;
float* floatOutputR = nullptr;

// Control state
ControlState controls;
AudioContext audioContext;

// Hardware state
EncoderState encoders[3];
SwitchState switches[3];

// Timing
uint32_t lastDisplayUpdate = 0;
const uint32_t DISPLAY_UPDATE_INTERVAL = 50; // 20 Hz

// ============================================================================
// AUDIO CALLBACK
// ============================================================================

void audioCallback(const float* inL, const float* inR,
                   float* outL, float* outR, int nFrames,
                   void* userData) {
  AudioContext* ctx = (AudioContext*)userData;
  ControlState* state = ctx->controls;

  const bool bypass = state->bypass;
  const Bank bank = state->bank;

  // Bypass mode
  if (bypass) {
    for (int i = 0; i < nFrames; ++i) {
      outL[i] = inL[i];
      outR[i] = inR[i];
    }
    return;
  }

  // Bank A: Saturation → DoubleTrack
  if (bank == BANK_A) {
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
// AUDIO ENGINE FUNCTIONS
// ============================================================================

void convertInt16ToFloat(const int16_t* src, float* dstL, float* dstR, int frames) {
  const float scale = 1.0f / 32768.0f;
  for (int i = 0; i < frames; i++) {
    dstL[i] = src[i * 2] * scale;
    dstR[i] = src[i * 2 + 1] * scale;
  }
}

void convertFloatToInt16(const float* srcL, const float* srcR, int16_t* dst, int frames) {
  for (int i = 0; i < frames; i++) {
    dst[i * 2] = (int16_t)constrain(srcL[i] * 32767.0f, -32768.0f, 32767.0f);
    dst[i * 2 + 1] = (int16_t)constrain(srcR[i] * 32767.0f, -32768.0f, 32767.0f);
  }
}

bool initAudioEngine() {
  // Allocate buffers in PSRAM
  i2sInputBuffer = (int16_t*)ps_malloc(BUFFER_SIZE * 2 * sizeof(int16_t));
  i2sOutputBuffer = (int16_t*)ps_malloc(BUFFER_SIZE * 2 * sizeof(int16_t));
  floatInputL = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));
  floatInputR = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));
  floatOutputL = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));
  floatOutputR = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));

  if (!i2sInputBuffer || !i2sOutputBuffer || !floatInputL ||
      !floatInputR || !floatOutputL || !floatOutputR) {
    Serial.println("ERROR: Failed to allocate audio buffers!");
    return false;
  }

  // I2S configuration for input (ADC)
  i2s_config_t i2s_config_in = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config_in = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DATA_IN_PIN
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config_in, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: Failed to install I2S RX driver!");
    return false;
  }

  if (i2s_set_pin(I2S_NUM_0, &pin_config_in) != ESP_OK) {
    Serial.println("ERROR: Failed to set I2S RX pins!");
    return false;
  }

  // I2S configuration for output (DAC)
  i2s_config_t i2s_config_out = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config_out = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DATA_OUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  if (i2s_driver_install(I2S_NUM_1, &i2s_config_out, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: Failed to install I2S TX driver!");
    return false;
  }

  if (i2s_set_pin(I2S_NUM_1, &pin_config_out) != ESP_OK) {
    Serial.println("ERROR: Failed to set I2S TX pins!");
    return false;
  }

  i2s_start(I2S_NUM_0);
  i2s_start(I2S_NUM_1);

  Serial.println("Audio engine initialized (48kHz, 128 samples)");
  return true;
}

void processAudio() {
  size_t bytesRead = 0;
  size_t bytesWritten = 0;

  // Read from ADC
  i2s_read(I2S_NUM_0, i2sInputBuffer, BUFFER_SIZE * 2 * sizeof(int16_t),
           &bytesRead, portMAX_DELAY);

  int framesRead = bytesRead / (2 * sizeof(int16_t));

  if (framesRead > 0) {
    // Convert to float
    convertInt16ToFloat(i2sInputBuffer, floatInputL, floatInputR, framesRead);

    // Process through DSP chain
    audioCallback(floatInputL, floatInputR, floatOutputL, floatOutputR,
                  framesRead, &audioContext);

    // Convert back to int16
    convertFloatToInt16(floatOutputL, floatOutputR, i2sOutputBuffer, framesRead);

    // Write to DAC
    i2s_write(I2S_NUM_1, i2sOutputBuffer, framesRead * 2 * sizeof(int16_t),
              &bytesWritten, portMAX_DELAY);
  }
}

// ============================================================================
// HARDWARE FUNCTIONS
// ============================================================================

void updateEncoder(EncoderState& enc) {
  uint8_t clk = digitalRead(enc.clkPin);
  uint8_t dt = digitalRead(enc.dtPin);
  uint8_t state = (clk << 1) | dt;

  if (state != enc.lastState) {
    uint8_t combined = (enc.lastState << 2) | state;

    // Gray code decoding
    if (combined == 0b0001 || combined == 0b0111 ||
        combined == 0b1110 || combined == 0b1000) {
      enc.delta++;
    } else if (combined == 0b0010 || combined == 0b1011 ||
               combined == 0b1101 || combined == 0b0100) {
      enc.delta--;
    }

    enc.lastState = state;
  }
}

void updateSwitch(SwitchState& sw) {
  uint8_t reading = digitalRead(sw.pin);

  if (reading != sw.lastReading) {
    sw.lastReading = reading;

    if (reading == LOW && !sw.pressed && !sw.held) {
      sw.pressed = true;
      sw.pressTime = millis();
    } else if (reading == HIGH) {
      sw.pressed = false;
      sw.held = false;
    }
  }

  if (sw.pressed && !sw.held && (millis() - sw.pressTime > 500)) {
    sw.held = true;
  }
}

void pollHardware() {
  for (int i = 0; i < 3; i++) {
    updateEncoder(encoders[i]);
    updateSwitch(switches[i]);
  }
}

bool consumeSwitchPress(int idx) {
  if (switches[idx].pressed && !switches[idx].held) {
    switches[idx].pressed = false;
    return true;
  }
  return false;
}

bool isSwitchHeld(int idx) {
  return switches[idx].held;
}

int consumeEncoderDelta(int idx) {
  if (idx < 0 || idx >= 3) return 0;
  int delta = encoders[idx].delta;
  encoders[idx].delta = 0;
  return delta;
}

void setLedColor(bool r, bool g, bool b) {
  digitalWrite(LED_R_PIN, r ? HIGH : LOW);
  digitalWrite(LED_G_PIN, g ? HIGH : LOW);
  digitalWrite(LED_B_PIN, b ? HIGH : LOW);
}

bool initHardware() {
  // Initialize encoders
  encoders[0] = {ENC1_CLK_PIN, ENC1_DT_PIN, 0, 0};
  encoders[1] = {ENC2_CLK_PIN, ENC2_DT_PIN, 0, 0};
  encoders[2] = {ENC3_CLK_PIN, ENC3_DT_PIN, 0, 0};

  for (int i = 0; i < 3; i++) {
    pinMode(encoders[i].clkPin, INPUT_PULLUP);
    pinMode(encoders[i].dtPin, INPUT_PULLUP);
    encoders[i].lastState = (digitalRead(encoders[i].clkPin) << 1) |
                             digitalRead(encoders[i].dtPin);
  }

  // Initialize switches
  switches[0] = {SW_BANK_PIN, false, false, 0, HIGH};
  switches[1] = {SW_BYPASS_PIN, false, false, 0, HIGH};
  switches[2] = {SW_SHIFT_PIN, false, false, 0, HIGH};

  for (int i = 0; i < 3; i++) {
    pinMode(switches[i].pin, INPUT_PULLUP);
  }

  // Initialize LED
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  setLedColor(false, false, false);

  Serial.println("Hardware initialized");
  return true;
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void drawParameter(int x, int y, const char* label, float value,
                   float minVal, float maxVal, const char* unit) {
  display.setTextSize(1);
  display.setCursor(x, y);
  display.print(label);
  display.print(": ");

  if (strcmp(unit, "%") == 0) {
    display.print((int)(value * 100));
  } else if (strcmp(unit, "Hz") == 0) {
    display.print(value, 1);
  } else {
    display.print(value, 1);
  }
  display.print(unit);

  // Progress bar
  int barX = x + 70;
  int barY = y + 1;
  int barWidth = 50;
  int barHeight = 5;

  display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  float normalized = (value - minVal) / (maxVal - minVal);
  normalized = constrain(normalized, 0.0f, 1.0f);
  int fillWidth = (int)(normalized * (barWidth - 2));

  if (fillWidth > 0) {
    display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);
  }
}

void updateDisplay(Bank bank, bool bypass,
                   float param1, float param2, float param3) {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (bypass) {
    display.print("BYPASS");
  } else if (bank == BANK_A) {
    display.print("BANK A - SAT");
  } else {
    display.print("BANK B - AMB");
  }

  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (!bypass) {
    if (bank == BANK_A) {
      drawParameter(0, 15, "DRIVE", param1, MIN_DRIVE, MAX_DRIVE, "dB");
      drawParameter(0, 32, "DBL MIX", param2, MIN_MIX, MAX_MIX, "%");
      drawParameter(0, 49, "LAG", param3, MIN_LAG_MS, MAX_LAG_MS, "ms");
    } else {
      drawParameter(0, 15, "TREM", param1, MIN_MIX, MAX_MIX, "%");
      drawParameter(0, 32, "REV MIX", param2, MIN_MIX, MAX_MIX, "%");
      drawParameter(0, 49, "RATE", param3, MIN_TREM_RATE_HZ, MAX_TREM_RATE_HZ, "Hz");
    }
  } else {
    display.setTextSize(2);
    display.setCursor(20, 30);
    display.print("BYPASSED");
  }

  display.display();
}

bool initDisplay() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 display init failed!");
    return false;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NASSAU LANE");
  display.println("Heltec V4");
  display.println("");
  display.println("Initializing...");
  display.display();

  Serial.println("Display initialized");
  return true;
}

// ============================================================================
// UTILITY
// ============================================================================

float clampFloat(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  NASSAU LANE - Heltec V4 Edition");
  Serial.println("  ESP32-S3 Guitar Effects Processor");
  Serial.println("========================================\n");

  // Initialize display
  if (!initDisplay()) {
    Serial.println("FATAL: Display failed!");
    while (1) delay(100);
  }

  // Initialize hardware
  if (!initHardware()) {
    Serial.println("FATAL: Hardware failed!");
    while (1) delay(100);
  }
  setLedColor(true, true, false); // Yellow = init

  // Allocate temp buffers
  audioContext.tempL = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));
  audioContext.tempR = (float*)ps_malloc(BUFFER_SIZE * sizeof(float));

  if (!audioContext.tempL || !audioContext.tempR) {
    Serial.println("FATAL: Temp buffer allocation failed!");
    while (1) delay(100);
  }

  // Initialize DSP
  Serial.println("Initializing DSP effects...");
  audioContext.sat.prepare(SAMPLE_RATE, BUFFER_SIZE);
  audioContext.dbl.prepare(SAMPLE_RATE, BUFFER_SIZE);
  audioContext.trem.prepare(SAMPLE_RATE, BUFFER_SIZE);
  audioContext.rev.prepare(SAMPLE_RATE, BUFFER_SIZE);
  audioContext.controls = &controls;

  // Initialize audio engine
  if (!initAudioEngine()) {
    Serial.println("FATAL: Audio engine failed!");
    while (1) delay(100);
  }

  setLedColor(false, true, true); // Cyan = ready

  Serial.println("\n========================================");
  Serial.println("  NASSAU LANE READY");
  Serial.println("========================================\n");
  Serial.println("Controls:");
  Serial.println("  SW1 (GPIO 45): Bank Toggle");
  Serial.println("  SW2 (GPIO 46): Bypass");
  Serial.println("  SW3 (GPIO 3):  Shift (hold)\n");
}

// ============================================================================
// ARDUINO MAIN LOOP
// ============================================================================

void loop() {
  // Process audio (blocking I2S read/write)
  processAudio();

  // Poll hardware
  pollHardware();

  // Handle switches
  if (consumeSwitchPress(0)) {
    controls.bank = (controls.bank == BANK_A) ? BANK_B : BANK_A;
    Serial.print("Bank: ");
    Serial.println((controls.bank == BANK_A) ? "A" : "B");
  }

  if (consumeSwitchPress(1)) {
    controls.bypass = !controls.bypass;
    Serial.print("Bypass: ");
    Serial.println(controls.bypass ? "ON" : "OFF");
  }

  // Shift LED
  if (isSwitchHeld(2)) {
    setLedColor(false, true, false); // Green
  } else {
    setLedColor(false, true, true); // Cyan
  }

  // Read encoders
  int d1 = consumeEncoderDelta(0);
  int d2 = consumeEncoderDelta(1);
  int d3 = consumeEncoderDelta(2);

  // Update parameters
  if (controls.bank == BANK_A) {
    if (d1 != 0) {
      float v = controls.driveDb;
      v = clampFloat(v + d1 * 0.5f, MIN_DRIVE, MAX_DRIVE);
      controls.driveDb = v;
    }
    if (d2 != 0) {
      float v = controls.doubleMix;
      v = clampFloat(v + d2 * 0.02f, MIN_MIX, MAX_MIX);
      controls.doubleMix = v;
    }
    if (d3 != 0) {
      float v = controls.lagMs;
      float w = controls.wobble;
      v = clampFloat(v + d3 * 0.5f, MIN_LAG_MS, MAX_LAG_MS);
      w = clampFloat(w + d3 * 0.02f, MIN_WOBBLE, MAX_WOBBLE);
      controls.lagMs = v;
      controls.wobble = w;
    }
  } else {
    if (d1 != 0) {
      float v = controls.tremDepth;
      v = clampFloat(v + d1 * 0.02f, MIN_MIX, MAX_MIX);
      controls.tremDepth = v;
    }
    if (d2 != 0) {
      float v = controls.revMix;
      v = clampFloat(v + d2 * 0.02f, MIN_MIX, MAX_MIX);
      controls.revMix = v;
    }
    if (d3 != 0) {
      float rate = controls.tremRate;
      float decay = controls.revDecay;
      rate = clampFloat(rate + d3 * 0.2f, MIN_TREM_RATE_HZ, MAX_TREM_RATE_HZ);
      decay = clampFloat(decay + d3 * 0.02f, MIN_DECAY, MAX_DECAY);
      controls.tremRate = rate;
      controls.revDecay = decay;
    }
  }

  // Update display
  uint32_t now = millis();
  if (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = now;

    if (controls.bank == BANK_A) {
      updateDisplay(controls.bank, controls.bypass,
                    controls.driveDb, controls.doubleMix, controls.lagMs);
    } else {
      updateDisplay(controls.bank, controls.bypass,
                    controls.tremDepth, controls.revMix, controls.tremRate);
    }
  }

  delayMicroseconds(100);
}
