# Nassau Lane ESP32 - Quick Start Guide

## 🚀 5-Minute Setup

### 1. Install Arduino IDE Libraries

Open Arduino IDE → Tools → Manage Libraries, then install:
- ✅ **Adafruit GFX Library**
- ✅ **Adafruit SSD1306**

### 2. Install ESP32 Board Support

File → Preferences → Additional Boards Manager URLs:
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Tools → Board → Boards Manager → Install **"esp32"**

### 3. Board Settings

- **Board:** Heltec WiFi LoRa 32(V3)
- **Upload Speed:** 921600
- **CPU Frequency:** 240MHz
- **PSRAM:** Enabled
- **Flash Size:** 8MB

### 4. Open & Upload

1. Open `NassauLaneESP32.ino` in Arduino IDE
2. Connect Heltec V4 via USB-C
3. Select correct COM port
4. Click Upload ⬆️

---

## 🎸 Minimal Wiring Test

Test audio without encoders/switches:

### PCM5102A (DAC Output)
```
Heltec V4          PCM5102A
─────────────      ────────────
GPIO 41     →      BCK
GPIO 42     →      LRCK/WS
GPIO 2      →      DIN
3.3V        →      VIN
GND         →      GND

Hardware Config:
  SCK  → GND
  XSMT → 3.3V
  FMT  → GND
```

### PCM1808 (ADC Input)
```
Heltec V4          PCM1808
─────────────      ────────────
GPIO 41     →      BCK
GPIO 42     →      LRCK
GPIO 1      →      DOUT
3.3V        →      VIN
GND         →      GND

Hardware Config:
  MD0 → GND
  MD1 → GND
```

---

## 🎛️ Add Controls (Optional)

### Single Test Switch (Bypass)
```
GPIO 46 ──┐
          │
        [SW] (Momentary)
          │
GND ──────┘
```

### Single Test Encoder
```
GPIO 4  → CLK
GPIO 5  → DT
3.3V    → +
GND     → GND
```

---

## 🐛 First Boot Checklist

### Serial Monitor (115200 baud) should show:
```
========================================
   NASSAU LANE - ESP32 Edition
   Heltec WiFi LoRa 32 V4
========================================

Display initialized
Hardware manager initialized
Initializing DSP effects...
Audio engine initialized successfully
Audio engine started

========================================
   NASSAU LANE READY
========================================
```

### OLED Display should show:
```
┌──────────────────────┐
│ BANK A - SAT        │
├──────────────────────┤
│ DRIVE: 4.0dB  [■──] │
│ DBL MIX: 30%  [■──] │
│ LAG: 12.0ms   [■──] │
└──────────────────────┘
```

### LED Status:
- **Yellow** during boot
- **Cyan** when ready

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| **"Display initialization failed"** | Normal - Heltec V4 display sometimes needs power cycle |
| **No audio output** | Check I2S wiring, verify 3.3V power is stable |
| **Crackling audio** | Increase buffer size in GlobalDefs.h (128→256) |
| **Upload fails** | Hold BOOT button during upload, check USB cable |

---

## 📊 Full Pinout Reference

See **README.md** for complete pinout tables.

### Quick Pin Summary:
- **I2S BCK:** GPIO 41
- **I2S WS:** GPIO 42
- **I2S OUT:** GPIO 2
- **I2S IN:** GPIO 1
- **Bank Switch:** GPIO 45
- **Bypass Switch:** GPIO 46
- **Shift Switch:** GPIO 3
- **Encoders:** GPIO 4-7, 15-16

---

## 🎵 Default Parameters

### Bank A (Saturation)
- Drive: 4 dB
- Mix: 30%
- Lag: 12 ms

### Bank B (Tremolo/Reverb)
- Tremolo Depth: 50%
- Reverb Mix: 20%
- Rate: 2 Hz

---

## 📝 Next Steps

1. ✅ Wire up audio I/O and test bypass mode
2. ✅ Add switches for bank toggle and bypass
3. ✅ Add encoders for parameter control
4. ✅ Connect RGB LED for status
5. 🎸 Rock out!

---

**Full documentation:** See README.md
**Issues?** Check Serial Monitor at 115200 baud
