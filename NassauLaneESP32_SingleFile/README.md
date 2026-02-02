# Nassau Lane - Heltec V4 Edition (Single File)

**Complete guitar effects processor in one Arduino sketch**

---

## 🎸 Quick Start

### 1. Install Libraries

Arduino IDE → Tools → Manage Libraries:
- **Adafruit GFX Library**
- **Adafruit SSD1306**

### 2. Install ESP32 Support

File → Preferences → Additional Board Manager URLs:
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Tools → Board Manager → Install **"esp32"** by Espressif

### 3. Configure Board

- **Board:** Heltec WiFi LoRa 32(V3) / Wireless shell(V3)
- **Upload Speed:** 921600
- **CPU Frequency:** 240MHz
- **Flash Size:** 8MB
- **Partition Scheme:** Default 4MB with spiffs
- **PSRAM:** Enabled ⚠️ IMPORTANT!

### 4. Upload

1. Open `NassauLaneESP32_SingleFile.ino`
2. Connect Heltec V4 via USB-C
3. Select COM port
4. Upload ⬆️

---

## 🔌 Wiring - Heltec V4 Specific

### I2S Audio (PCM5102A + PCM1808)

```
Heltec V4          Audio Modules
──────────────     ──────────────
GPIO 41    →       BCK (both boards)
GPIO 42    →       LRCK/WS (both boards)
GPIO 2     →       DIN (PCM5102 DAC)
GPIO 1     →       DOUT (PCM1808 ADC)
3.3V       →       VIN (both boards)
GND        →       GND (both boards)
```

**PCM5102A Config (Slave Mode):**
- SCK → GND
- XSMT → 3.3V
- FMT → GND

**PCM1808 Config (Slave Mode):**
- MD0 → GND
- MD1 → GND

### Control Inputs

**Footswitches** (Momentary, connect to GND):
```
GPIO 45 → Bank Toggle (A/B)
GPIO 46 → Bypass
GPIO 3  → Shift (hold for alt functions)
```

**Rotary Encoders:**
```
Encoder 1: CLK=GPIO 4,  DT=GPIO 5  (Drive/Tremolo)
Encoder 2: CLK=GPIO 6,  DT=GPIO 7  (Mix)
Encoder 3: CLK=GPIO 15, DT=GPIO 16 (Lag/Rate)
```

**RGB LED** (Common Cathode):
```
GPIO 47 → Red
GPIO 48 → Green
GPIO 35 → Blue
```

**OLED Display:**
- Built-in, auto-configured (GPIO 17/18)

---

## 🎛️ Controls

### Bank A - Saturation Mode
| Encoder | Parameter | Range |
|---------|-----------|-------|
| **1** | Saturation Drive | 0 - 26 dB |
| **2** | DoubleTrack Mix | 0 - 100% |
| **3** | Lag Time & Wobble | 4 - 40 ms |

**Chain:** Input → Saturation → DoubleTrack → Output

### Bank B - Ambience Mode
| Encoder | Parameter | Range |
|---------|-----------|-------|
| **1** | Tremolo Depth | 0 - 100% |
| **2** | Reverb Mix | 0 - 100% |
| **3** | Tremolo Rate & Decay | 0.2 - 8 Hz |

**Chain:** Input → Tremolo → Reverb → Output

---

## 🔍 Serial Monitor (115200 baud)

Expected output on successful boot:
```
========================================
  NASSAU LANE - Heltec V4 Edition
  ESP32-S3 Guitar Effects Processor
========================================

Display initialized
Hardware initialized
Initializing DSP effects...
Audio engine initialized (48kHz, 128 samples)

========================================
  NASSAU LANE READY
========================================
```

---

## 💡 LED Status

- **Yellow** - Initializing
- **Cyan** - Running (normal)
- **Green** - Shift button held

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| **Won't compile** | Enable PSRAM in board settings! |
| **Display blank** | Normal - power cycle Heltec V4 |
| **No audio** | Check I2S wiring, verify BCK/WS shared |
| **Crackling** | Use quality USB power, check grounds |
| **Upload fails** | Hold BOOT button, try lower upload speed |

---

## ⚙️ Customization

All settings are `#define` constants at the top of the sketch:

```cpp
// Change pins
#define I2S_BCK_PIN         41
#define SW_BANK_PIN         45

// Adjust audio
#define SAMPLE_RATE         48000
#define BUFFER_SIZE         128

// Tweak parameters
#define MAX_DRIVE           26.0f
#define MIN_LAG_MS          4.0f
```

---

## 📊 Performance

- **Sample Rate:** 48 kHz
- **Latency:** ~2.67 ms (128 samples)
- **CPU Usage:** ~50-70% @ 240MHz
- **RAM Usage:** ~60 KB (PSRAM)
- **Buffer Memory:** ~27.5 KB reverb + 15 KB delay

---

## 📝 Features

✅ Dual processing banks with instant switching
✅ Real-time OLED parameter visualization
✅ Hardware bypass mode
✅ Smooth parameter interpolation (no zipper noise)
✅ Gray-code encoder decoding (reliable)
✅ Switch debouncing with hold detection
✅ PSRAM allocation for large DSP buffers
✅ Professional audio quality (32-bit float DSP)

---

## 🎓 Technical Details

**DSP Chain:**
- All processing in 32-bit float
- Per-sample processing (no block artifacts)
- Smooth parameter ramping
- Comb filter reverb (2 combs + 1 allpass)
- Modulated delay for chorus/doubletrack
- Sine wave LFO for tremolo

**Audio Path:**
- I2S Master mode (ESP32 generates clocks)
- Dual I2S peripherals (I2S_NUM_0 for RX, I2S_NUM_1 for TX)
- 16-bit I2S → 32-bit float → DSP → 32-bit float → 16-bit I2S
- DMA-based transfers (low CPU overhead)

---

## 📄 License

Copyright © 2026 Parker Redding. All Rights Reserved.

---

## 🔗 Original Project

Ported from Raspberry Pi Zero 2 W version (Linux/ALSA)
Platform: ESP32-S3 (Heltec WiFi LoRa 32 V4)
Format: Single-file Arduino sketch for easy sharing
