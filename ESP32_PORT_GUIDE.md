# Nassau Lane ESP32 Ports - Quick Guide

This repository contains **two ESP32 ports** of the Nassau Lane effects processor. Choose the one that fits your workflow:

---

## 📁 Version Comparison

| Feature | Multi-File Version | Single-File Version |
|---------|-------------------|---------------------|
| **Location** | `NassauLaneESP32/` | `NassauLaneESP32_SingleFile/` |
| **Files** | 18 files (.h/.cpp) | 1 file (.ino) |
| **Organization** | Structured (classes in separate files) | Monolithic (everything in one file) |
| **Best For** | Development, modification | Quick deployment, sharing |
| **Readability** | Better (organized by module) | Good (1200 lines, well-commented) |
| **Portability** | Requires folder structure | Copy one file and go |

---

## 🎯 Which Version Should I Use?

### Choose **Multi-File** (`NassauLaneESP32/`) if:
- ✅ You plan to modify or extend the code
- ✅ You prefer organized, modular code
- ✅ You're familiar with C++ header/source files
- ✅ You want to add new effects easily

### Choose **Single-File** (`NassauLaneESP32_SingleFile/`) if:
- ✅ You just want to upload and use it
- ✅ You're sharing with others (easier to distribute)
- ✅ You prefer Arduino-style single sketches
- ✅ You don't plan to modify the DSP algorithms

---

## ⚡ Quick Start (Either Version)

### Both Versions Require:

**Libraries:**
- Adafruit GFX Library
- Adafruit SSD1306

**Board Settings:**
- Board: Heltec WiFi LoRa 32(V3)
- CPU: 240MHz
- **PSRAM: Enabled** ⚠️ Critical!

**Hardware:**
- Heltec WiFi LoRa 32 V4
- PCM5102A DAC + PCM1808 ADC
- 3x Rotary Encoders
- 3x Momentary Switches
- 1x RGB LED

---

## 🔌 Pinout (Same for Both Versions)

### I2S Audio
```
GPIO 41 → BCK (Bit Clock)
GPIO 42 → WS (Word Select)
GPIO 2  → Data Out (to DAC)
GPIO 1  → Data In (from ADC)
```

### Controls
```
GPIO 45 → Bank Switch
GPIO 46 → Bypass Switch
GPIO 3  → Shift Switch

GPIO 4/5   → Encoder 1 (CLK/DT)
GPIO 6/7   → Encoder 2 (CLK/DT)
GPIO 15/16 → Encoder 3 (CLK/DT)

GPIO 47/48/35 → RGB LED (R/G/B)
```

### OLED Display
```
Built-in on Heltec V4 (auto-configured)
GPIO 17 → SDA
GPIO 18 → SCL
```

---

## 📊 Technical Specs (Identical)

Both versions implement the same DSP:

- **Sample Rate:** 48 kHz
- **Latency:** ~2.67 ms (128 samples)
- **CPU Usage:** 50-70% @ 240MHz
- **Memory:** ~60 KB (PSRAM)

**Bank A:** Saturation → DoubleTrack
**Bank B:** Tremolo → Reverb

---

## 🎸 Features (Identical)

✅ Dual processing banks
✅ Real-time OLED display
✅ Hardware bypass
✅ Smooth parameters (no zipper noise)
✅ Professional 32-bit float DSP
✅ Gray-code encoder decoding
✅ Debounced switches with hold detection

---

## 🔧 Customization

### Multi-File Version:
- Edit `GlobalDefs.h` for pins/constants
- Modify individual effect files in `Effects/`
- Clear separation of concerns

### Single-File Version:
- Edit `#define` constants at top of .ino
- All code in one file (1200 lines)
- Quick search/replace

---

## 📚 Documentation

**Multi-File:**
- `NassauLaneESP32/README.md` - Complete guide
- `NassauLaneESP32/QUICK_START.md` - 5-minute setup

**Single-File:**
- `NassauLaneESP32_SingleFile/README.md` - All-in-one guide

---

## 🚀 Original Project

Both ports are based on the original **Raspberry Pi Zero 2 W** version in the root directory, which uses:
- Linux + ALSA for audio
- C++17 with multithreading
- Direct I2S hardware control

The ESP32 ports maintain 100% identical DSP algorithms, only the platform layer (audio I/O, GPIO) was rewritten.

---

## 💾 File Sizes

**Multi-File:** ~18 files, ~1500 total lines
**Single-File:** 1 file, ~1200 lines

Both compile to the same ~400KB binary.

---

## ✅ Recommendation

**First time user?** → Start with **Single-File** version
**Planning to customize?** → Use **Multi-File** version

Both are fully functional and production-ready!
