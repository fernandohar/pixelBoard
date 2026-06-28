#ifndef _PIXELMOOD_H
#define _PIXELMOOD_H

#include "pixelBoardBase.h"

#define MOOD_PRESET_COUNT 14

class PixelMood : public PixelBoardBase {
public:
  PixelMood(){};
  PixelMood(Adafruit_NeoPixel* stripPtr) : PixelBoardBase(stripPtr){};

  void update(unsigned long currentMillis);
  void reset();
  void nextPreset();
  void previousPreset();
  void setPreset(byte preset);
  byte getPreset();

private:
  byte currentPreset = 0;
  byte animationOffset = 0;

  void renderSolid(byte red, byte green, byte blue);
  void renderRainbowDiagonal();
  void renderRainbowBlocks();
  void renderRandomPixels();
  void renderRandomSparkle();
  uint32_t wheel(byte wheelPos);
};

#endif
