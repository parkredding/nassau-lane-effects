#include "DisplayManager.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

DisplayManager::DisplayManager() : display_(nullptr) {}

bool DisplayManager::init() {
  // Initialize I2C for OLED (Heltec V4)
  Wire.begin(defs::kOledSDA, defs::kOledSCL);

  display_ = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, defs::kOledRST);

  if (!display_->begin(SSD1306_SWITCHCAPVCC, defs::kOledAddress)) {
    Serial.println("SSD1306 allocation failed");
    return false;
  }

  display_->clearDisplay();
  display_->setTextSize(1);
  display_->setTextColor(SSD1306_WHITE);
  display_->setCursor(0, 0);
  display_->println("NASSAU LANE");
  display_->println("ESP32");
  display_->println("");
  display_->println("Initializing...");
  display_->display();

  Serial.println("Display initialized");
  return true;
}

void DisplayManager::update(defs::Bank bank, bool bypass,
                            float param1, float param2, float param3) {
  if (!display_) return;

  display_->clearDisplay();

  // Title bar
  display_->setTextSize(1);
  display_->setCursor(0, 0);
  if (bypass) {
    display_->print("BYPASS");
  } else if (bank == defs::Bank::A) {
    display_->print("BANK A - SAT");
  } else {
    display_->print("BANK B - AMB");
  }

  // Draw line separator
  display_->drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (!bypass) {
    if (bank == defs::Bank::A) {
      drawBankA(param1, param2, param3);
    } else {
      drawBankB(param1, param2, param3);
    }
  } else {
    display_->setTextSize(2);
    display_->setCursor(20, 30);
    display_->print("BYPASSED");
  }

  display_->display();
}

void DisplayManager::drawBankA(float drive, float mix, float lag) {
  // Drive (0-26 dB)
  drawParameter(0, 15, "DRIVE", drive, defs::kMinDrive, defs::kMaxDrive, "dB");

  // Mix (0-1)
  drawParameter(0, 32, "DBL MIX", mix, defs::kMinMix, defs::kMaxMix, "%");

  // Lag (4-40 ms)
  drawParameter(0, 49, "LAG", lag, defs::kMinLagMs, defs::kMaxLagMs, "ms");
}

void DisplayManager::drawBankB(float depth, float revMix, float rate) {
  // Tremolo Depth (0-1)
  drawParameter(0, 15, "TREM", depth, defs::kMinMix, defs::kMaxMix, "%");

  // Reverb Mix (0-1)
  drawParameter(0, 32, "REV MIX", revMix, defs::kMinMix, defs::kMaxMix, "%");

  // Rate (0.2-8 Hz)
  drawParameter(0, 49, "RATE", rate, defs::kMinTremRateHz, defs::kMaxTremRateHz, "Hz");
}

void DisplayManager::drawParameter(int x, int y, const char* label, float value,
                                   float minVal, float maxVal, const char* unit) {
  display_->setTextSize(1);
  display_->setCursor(x, y);
  display_->print(label);
  display_->print(": ");

  // Format value based on unit
  if (strcmp(unit, "%") == 0) {
    display_->print((int)(value * 100));
  } else if (strcmp(unit, "Hz") == 0) {
    display_->print(value, 1);
  } else {
    display_->print(value, 1);
  }
  display_->print(unit);

  // Draw progress bar
  int barX = x + 70;
  int barY = y + 1;
  int barWidth = 50;
  int barHeight = 5;

  display_->drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  float normalized = (value - minVal) / (maxVal - minVal);
  normalized = constrain(normalized, 0.0f, 1.0f);
  int fillWidth = (int)(normalized * (barWidth - 2));

  if (fillWidth > 0) {
    display_->fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);
  }
}

void DisplayManager::clear() {
  if (display_) {
    display_->clearDisplay();
    display_->display();
  }
}
