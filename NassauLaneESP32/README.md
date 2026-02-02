# Nassau Lane ESP32 - Guitar Effects Processor

**ESP32-S3 Port for Heltec WiFi LoRa 32 V4**

High-fidelity dual-engine digital guitar effects processor with OLED display.

---

## Features

- **Dual Processing Banks:**
  - **Bank A:** Tape Saturation + Double Tracking
  - **Bank B:** Harmonic Tremolo + Reverb
- **48kHz Sample Rate** with 128-sample buffer
- **128x64 OLED Display** with real-time parameter visualization
- **3 Rotary Encoders** for intuitive control
- **3 Footswitches** (Bank Toggle, Bypass, Shift)
- **RGB LED** status indicator

---

## Required Libraries

Install these libraries via Arduino Library Manager:

1. **Adafruit GFX Library** (by Adafruit)
2. **Adafruit SSD1306** (by Adafruit)
3. **ESP32 Board Support** (via Boards Manager)

### Installing ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "ESP32" and install **"esp32" by Espressif Systems** (v2.0.11 or later)
6. Select **Tools → Board → ESP32 Arduino → Heltec WiFi LoRa 32(V3) / Wireless shell(V3) / Wireless stick lite (V3)**

---

## Hardware Setup

### Audio Interface Modules

**PCM5102A DAC (Output)** - Configure as I2S Slave:
- SCK → GND (forces internal PLL)
- XSMT → 3.3V (unmutes output)
- FMT → GND (I2S format)

**PCM1808 ADC (Input)** - Configure as I2S Slave:
- MD0/MD1 → GND (Slave mode)
- FMT → VCC (I2S format)

### Complete Pinout

#### I2S Audio Bus

| Function | ESP32 Pin | PCM5102/PCM1808 Pin | Notes |
|----------|-----------|---------------------|-------|
| BCK (Bit Clock) | GPIO 41 | BCK on both | Shared clock |
| WS (Word Select) | GPIO 42 | LRCK on both | Shared clock |
| Data Out | GPIO 2 | DIN (PCM5102) | To DAC |
| Data In | GPIO 1 | DOUT (PCM1808) | From ADC |
| VCC | 3.3V | VIN on both | Power |
| GND | GND | GND on both | Ground |

#### Footswitches (Momentary, Active LOW)

| Switch | ESP32 Pin | Function |
|--------|-----------|----------|
| SW1 | GPIO 45 | Bank Toggle (A/B) |
| SW2 | GPIO 46 | Bypass |
| SW3 | GPIO 3 | Shift (Hold) |

*Note: Switches connect between GPIO and GND (internal pull-ups enabled)*

#### Rotary Encoders

| Encoder | CLK Pin | DT Pin | Function |
|---------|---------|--------|----------|
| ENC1 | GPIO 4 | GPIO 5 | Drive / Tremolo Depth |
| ENC2 | GPIO 6 | GPIO 7 | Mix / Reverb Mix |
| ENC3 | GPIO 15 | GPIO 16 | Lag/Wobble / Rate/Decay |

*Note: Use standard rotary encoders with internal detents*

#### RGB LED

| LED Channel | ESP32 Pin |
|-------------|-----------|
| Red | GPIO 47 |
| Green | GPIO 48 |
| Blue | GPIO 21 |

*Note: Common cathode RGB LED*

#### OLED Display (Built-in on Heltec V4)

| Function | ESP32 Pin |
|----------|-----------|
| SDA | GPIO 17 |
| SCL | GPIO 18 |
| RST | GPIO 21 |

*Note: Display is built-in and auto-configured*

---

## Installation

1. **Download the NassauLaneESP32 folder**
2. **Place it in your Arduino sketchbook folder:**
   - Windows: `Documents\Arduino\NassauLaneESP32\`
   - macOS: `~/Documents/Arduino/NassauLaneESP32/`
   - Linux: `~/Arduino/NassauLaneESP32/`
3. **Install required libraries** (see above)
4. **Open `NassauLaneESP32.ino` in Arduino IDE**
5. **Select board:** Tools → Board → Heltec WiFi LoRa 32(V3)
6. **Configure settings:**
   - Upload Speed: 921600
   - CPU Frequency: 240MHz
   - Flash Size: 8MB
   - Partition Scheme: Default 4MB with spiffs
   - PSRAM: Enabled
7. **Connect Heltec V4 via USB-C**
8. **Upload the sketch**

---

## Control Mapping

### Bank A - Saturation Mode

| Control | Parameter | Range |
|---------|-----------|-------|
| **ENC1** | Saturation Drive | 0 - 26 dB |
| **ENC2** | DoubleTrack Mix | 0 - 100% |
| **ENC3** | Lag Time & Wobble | 4 - 40 ms |

**Signal Flow:** Input → Saturation → DoubleTrack → Output

### Bank B - Ambience Mode

| Control | Parameter | Range |
|---------|-----------|-------|
| **ENC1** | Tremolo Depth | 0 - 100% |
| **ENC2** | Reverb Mix | 0 - 100% |
| **ENC3** | Tremolo Rate & Decay | 0.2 - 8 Hz |

**Signal Flow:** Input → Tremolo → Reverb → Output

### LED Status Indicators

- **Yellow** - Initializing
- **Cyan** - Running normally
- **Green** - Shift button held

---

## OLED Display

The display shows:
- **Current bank** (A or B)
- **Bypass status**
- **Three active parameters** with values and progress bars
- Updates at 20 Hz for smooth visualization

---

## Serial Monitor Output

Connect to Serial Monitor (115200 baud) to see:
- Initialization status
- Bank switching events
- Bypass toggling
- Real-time parameter updates (when debug enabled)

---

## Memory Usage

Approximate RAM usage:
- **Reverb buffers:** ~27.5 KB (PSRAM)
- **DoubleTrack delay:** ~15 KB (PSRAM)
- **I2S buffers:** ~4 KB
- **Display buffer:** ~1 KB
- **Total:** ~50-60 KB (well within 8MB PSRAM)

---

## Performance Notes

- **Sample Rate:** 48 kHz
- **Buffer Size:** 128 samples (2.67ms latency)
- **CPU Usage:** ~50-70% @ 240MHz
- **Cores Used:**
  - Core 0: Arduino loop, display, controls
  - Core 1: I2S processing (implicit)

---

## Troubleshooting

### No Audio Output
- Check I2S wiring (BCK, WS, Data Out)
- Verify PCM5102A is configured as slave (SCK→GND, FMT→GND)
- Check Serial Monitor for initialization errors

### Crackling/Distorted Audio
- Ensure stable 3.3V power supply
- Check ground connections
- Verify sample rate matches (48kHz)
- Try increasing buffer size in GlobalDefs.h

### Display Not Working
- Built-in OLED should auto-detect
- Check Serial Monitor for "Display initialized"
- Verify I2C address (0x3C)

### Encoders Not Responding
- Check wiring (CLK and DT pins)
- Verify internal pull-ups are working
- Try swapping CLK/DT if direction is reversed

### Switches Not Working
- Switches should be normally open (NO)
- Connect between GPIO and GND
- Check Serial Monitor for switch events

---

## Customization

### Adjusting Parameters

Edit **GlobalDefs.h** to change:
- Pin assignments
- Sample rate (24kHz, 44.1kHz, 48kHz)
- Buffer size (64, 128, 256)
- Parameter ranges

### Adding New Effects

1. Create effect class in `Effects/` folder
2. Inherit from `EffectBase`
3. Implement `prepare()` and `process()` methods
4. Add to audio processing chain in main `.ino` file

---

## Credits

**Original Design:** Parker Redding
**Platform:** Raspberry Pi Zero 2 W → ESP32-S3 Port
**License:** Proprietary. Copyright © 2026 Parker Redding. All Rights Reserved.

---

## Version History

- **v1.0** - Initial ESP32-S3 port for Heltec V4
  - Complete DSP chain ported
  - OLED display support added
  - I2S audio engine optimized for ESP32
