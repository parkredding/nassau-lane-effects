# Nassau Lane Effects Processor

**Version:** 0.1.0-alpha
**Platform:** Raspberry Pi Zero 2 W (RPi OS Lite)
**Language:** C++
**License:** Proprietary. Copyright © 2026 Parker Redding. All Rights Reserved.

## 1. Overview

The **Nassau Lane** is a high-fidelity, dual-engine digital signal processor designed for guitar and line-level audio. It functions as a standalone digital effects unit, offering two distinct processing banks that can be toggled via a momentary footswitch.

The architecture is built on the Raspberry Pi Zero 2 W using direct I2S audio handling for low-latency performance.

### The Two Banks
The pedal features two distinct "Personalities" (Banks). The control surface functions change depending on the active bank, but every encoder remains mapped to a critical parameter in both modes to ensure the interface is always active.

* **Bank A (Saturation):** Focuses on vintage tape saturation, compression, and double-tracking effects.
* **Bank B (Ambience):** Focuses on harmonic tremolo and various reverb algorithms.

---

## 2. Hardware Configuration

### Audio Interface Modules
* **Audio Output (DAC):** PCM5102A ("Purple Board")
* **Audio Input (ADC):** PCM1808 ("Purple Board")

### "Purple Board" Hardwiring Requirements
**CRITICAL:** Both the ADC and DAC modules must be configured as **Slaves** so the Raspberry Pi can act as the I2S Master.

**1. PCM5102 (DAC) Settings:**
* **SCK:** Connect to **GND** (Forces internal PLL).
* **XSMT:** Connect to **3.3V** (Unmutes the output).
* **FMT:** Connect to **GND** (Sets I2S format).

**2. PCM1808 (ADC) Settings:**
* **MD0 / MD1:** Ensure these are set to **Slave Mode** (usually pulled Low/GND on purple boards, check your specific breakout).
* **FMT:** Ensure set to **I2S Data Format** (often High/VCC on these boards, verify against breakout datasheet).

---

### 3. Pinout Tables

#### Table A: I2S Audio Bus (DAC & ADC)
*Note: The Clock lines (BCK and LRCK) are shared between the Input and Output boards.*

| Pin Name | Function | Header Position | GPIO # (BCM) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **BCK** | Bit Clock | **12** | **18** | Shared (Connect to both boards) |
| **LRCK** | Word Clock | **35** | **19** | Shared (Connect to both boards) |
| **DIN** | Data In | **38** | **20** | **From** PCM1808 DOUT pin |
| **DOUT** | Data Out | **40** | **21** | **To** PCM5102 DIN pin |
| **5V** | Power | **2** | **-** | Power for DAC & ADC |
| **GND** | Ground | **6** | **-** | Common Ground |

#### Table B: Momentary Footswitches
*Note: Switches connect between the listed GPIO and Ground.*

| Switch Name | Function | Header Position | GPIO # (BCM) |
| :--- | :--- | :--- | :--- |
| **SW_BANK** | Toggle Banks (A/B) | **8** | **14** |
| **SW_BYPASS**| Master Bypass | **10** | **15** |
| **SW_SHIFT** | Shift (Hold) | **36** | **16** |

#### Table C: Rotary Encoders (DT/CLK)
*Note: Assumes 4-pin Breakout Modules (VCC, GND, CLK, DT).*

| Encoder | Function | VCC (3.3V) | GND | CLK Pin | DT Pin |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **ENC_1** | Saturation / Tremolo | **Pin 1** | **Pin 9** | **GPIO 2** (Pin 3) | **GPIO 3** (Pin 5) |
| **ENC_2** | Doubletrack / Reverb | **Pin 17** | **Pin 25** | **GPIO 4** (Pin 7) | **GPIO 5** (Pin 29) |
| **ENC_3** | Lag / Decay | **Pin 1** | **Pin 39** | **GPIO 6** (Pin 31) | **GPIO 7** (Pin 26) |

#### Table D: RGB LED (Assignable)
| LED Channel | Function | Header Position | GPIO # (BCM) |
| :--- | :--- | :--- | :--- |
| **LED_R** | Red Channel | **11** | **17** |
| **LED_G** | Green Channel | **13** | **27** |
| **LED_B** | Blue Channel | **15** | **22** |

---

## 4. Control Surface Mapping

The **Nassau Lane** uses a "Context-Aware" control scheme.

### Bank Toggle
* **Switch 1 (Momentary):** Toggles between Bank A (Saturation) and Bank B (Ambience).
* **Switch 2 (Momentary):** True Bypass (Software or Relay based).

### Encoder Functions

| Control | **Bank A (Saturation)** | **Bank B (Ambience)** |
| :--- | :--- | :--- |
| **Encoder 1** | **Saturation Drive** <br> *Controls input gain and tape compression.* | **Tremolo Intensity** <br> *Controls the depth of the volume modulation.* |
| **Encoder 2** | **Doubletracker Mix** <br> *Blends the secondary "lag" signal.* | **Reverb Mix** <br> *Blends the wet reverb signal with the dry path.* |
| **Encoder 3** | **Lag Time / Wobble** <br> *Sets delay offset and modulation.* | **Speed / Decay** <br> *Controls Tremolo LFO speed and Reverb Decay time.* |

---

## 5. File Structure

The project is organized to separate core audio handling, hardware interfacing, and DSP algorithms.

```text
nassau-lane/
├── README.md               # Project documentation and pinout
├── LICENSE                 # Proprietary License (Parker Redding, 2026)
├── Makefile                # Compilation instructions
├── nassau_lane.service     # Systemd unit file for headless auto-start
├── scripts/
│   └── gpio_init.sh        # Shell script to set initial pin states
├── include/
│   ├── AudioEngine.h       # ALSA setup and callback handling
│   ├── HardwareManager.h   # GPIO, Encoder, and Switch handling classes
│   ├── GlobalDefs.h        # Constants, sample rates, and pin definitions
│   └── Effects/
│       ├── EffectBase.h    # Base class for all effects
│       ├── Saturation.h    # Bank A: Saturation algorithm
│       ├── DoubleTrack.h   # Bank A: Modulation algorithm
│       ├── Tremolo.h       # Bank B: Tremolo algorithm
│       └── Reverb.h        # Bank B: Reverb algorithm
└── src/
    ├── main.cpp            # Entry point; initializes threads
    ├── AudioEngine.cpp     # Manages I2S buffers and processes audio blocks
    ├── HardwareManager.cpp # Debouncing logic and encoder state tracking
    └── Effects/
        ├── Saturation.cpp
        ├── DoubleTrack.cpp
        ├── Tremolo.cpp
        └── Reverb.cpp
