#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "GlobalDefs.h"

class DisplayManager {
 public:
  DisplayManager();
  bool init();
  void update(defs::Bank bank, bool bypass,
              float param1, float param2, float param3);
  void clear();

 private:
  Adafruit_SSD1306* display_;

  void drawBankA(float drive, float mix, float lag);
  void drawBankB(float depth, float revMix, float rate);
  void drawParameter(int x, int y, const char* label, float value,
                     float minVal, float maxVal, const char* unit);
};
