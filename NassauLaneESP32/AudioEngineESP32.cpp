#include "AudioEngineESP32.h"
#include <Arduino.h>

AudioEngineESP32::AudioEngineESP32() {}

AudioEngineESP32::~AudioEngineESP32() {
  stop();
  if (i2sInputBuffer_) free(i2sInputBuffer_);
  if (i2sOutputBuffer_) free(i2sOutputBuffer_);
  if (floatInputL_) free(floatInputL_);
  if (floatInputR_) free(floatInputR_);
  if (floatOutputL_) free(floatOutputL_);
  if (floatOutputR_) free(floatOutputR_);
}

bool AudioEngineESP32::init() {
  // Allocate buffers
  i2sInputBuffer_ = (int16_t*)malloc(defs::kBufferSize * 2 * sizeof(int16_t));
  i2sOutputBuffer_ = (int16_t*)malloc(defs::kBufferSize * 2 * sizeof(int16_t));
  floatInputL_ = (float*)malloc(defs::kBufferSize * sizeof(float));
  floatInputR_ = (float*)malloc(defs::kBufferSize * sizeof(float));
  floatOutputL_ = (float*)malloc(defs::kBufferSize * sizeof(float));
  floatOutputR_ = (float*)malloc(defs::kBufferSize * sizeof(float));

  if (!i2sInputBuffer_ || !i2sOutputBuffer_ || !floatInputL_ ||
      !floatInputR_ || !floatOutputL_ || !floatOutputR_) {
    Serial.println("Failed to allocate audio buffers!");
    return false;
  }

  // Configure I2S for input (ADC - PCM1808)
  i2s_config_t i2s_config_in = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = defs::kSampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = defs::kBufferSize,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config_in = {
    .bck_io_num = defs::kI2S_BCK,
    .ws_io_num = defs::kI2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = defs::kI2S_DATA_IN
  };

  // Install and start I2S driver for RX
  if (i2s_driver_install(I2S_NUM_0, &i2s_config_in, 0, NULL) != ESP_OK) {
    Serial.println("Failed to install I2S driver for RX!");
    return false;
  }

  if (i2s_set_pin(I2S_NUM_0, &pin_config_in) != ESP_OK) {
    Serial.println("Failed to set I2S pins for RX!");
    return false;
  }

  // Configure I2S for output (DAC - PCM5102)
  i2s_config_t i2s_config_out = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = defs::kSampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = defs::kBufferSize,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config_out = {
    .bck_io_num = defs::kI2S_BCK,
    .ws_io_num = defs::kI2S_WS,
    .data_out_num = defs::kI2S_DATA_OUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Install and start I2S driver for TX
  if (i2s_driver_install(I2S_NUM_1, &i2s_config_out, 0, NULL) != ESP_OK) {
    Serial.println("Failed to install I2S driver for TX!");
    return false;
  }

  if (i2s_set_pin(I2S_NUM_1, &pin_config_out) != ESP_OK) {
    Serial.println("Failed to set I2S pins for TX!");
    return false;
  }

  Serial.println("Audio engine initialized successfully");
  return true;
}

void AudioEngineESP32::setCallback(AudioCallback cb, void* userData) {
  callback_ = cb;
  userData_ = userData;
}

bool AudioEngineESP32::start() {
  if (!callback_) {
    Serial.println("No audio callback set!");
    return false;
  }

  i2s_start(I2S_NUM_0);
  i2s_start(I2S_NUM_1);
  running_ = true;
  Serial.println("Audio engine started");
  return true;
}

void AudioEngineESP32::stop() {
  if (running_) {
    i2s_stop(I2S_NUM_0);
    i2s_stop(I2S_NUM_1);
    running_ = false;
  }
}

void AudioEngineESP32::process() {
  if (!running_ || !callback_) return;

  size_t bytesRead = 0;
  size_t bytesWritten = 0;

  // Read from ADC (I2S_NUM_0)
  i2s_read(I2S_NUM_0, i2sInputBuffer_, defs::kBufferSize * 2 * sizeof(int16_t),
           &bytesRead, portMAX_DELAY);

  int framesRead = bytesRead / (2 * sizeof(int16_t));

  if (framesRead > 0) {
    // Convert int16 to float
    convertInt16ToFloat(i2sInputBuffer_, floatInputL_, floatInputR_, framesRead);

    // Process audio through callback
    callback_(floatInputL_, floatInputR_, floatOutputL_, floatOutputR_,
              framesRead, userData_);

    // Convert float to int16
    convertFloatToInt16(floatOutputL_, floatOutputR_, i2sOutputBuffer_, framesRead);

    // Write to DAC (I2S_NUM_1)
    i2s_write(I2S_NUM_1, i2sOutputBuffer_, framesRead * 2 * sizeof(int16_t),
              &bytesWritten, portMAX_DELAY);
  }
}

void AudioEngineESP32::convertInt16ToFloat(const int16_t* src, float* dstL,
                                           float* dstR, int frames) {
  const float scale = 1.0f / 32768.0f;
  for (int i = 0; i < frames; i++) {
    dstL[i] = src[i * 2] * scale;
    dstR[i] = src[i * 2 + 1] * scale;
  }
}

void AudioEngineESP32::convertFloatToInt16(const float* srcL, const float* srcR,
                                           int16_t* dst, int frames) {
  for (int i = 0; i < frames; i++) {
    dst[i * 2] = (int16_t)constrain(srcL[i] * 32767.0f, -32768.0f, 32767.0f);
    dst[i * 2 + 1] = (int16_t)constrain(srcR[i] * 32767.0f, -32768.0f, 32767.0f);
  }
}
