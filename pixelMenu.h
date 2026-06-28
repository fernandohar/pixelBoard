#ifndef PIXEL_MENU_H
#define PIXEL_MENU_H

#include "pixelBoardBase.h"

class PixelMenu : public PixelBoardBase {
public:
  PixelMenu(){};
  PixelMenu(Adafruit_NeoPixel* stripPtr) : PixelBoardBase(stripPtr){};

  void update(unsigned long currentMillis);
  void reset();
  void showText(const String& text, unsigned long currentMillis, uint32_t color = WHITE);

private:
  String currentText = "";
  uint16_t scrollOffset = 0;
  unsigned long lastScroll = 0;

  void clear();
  void drawText(const String& text, int16_t x, byte y, uint32_t color);
  void drawChar(char c, int16_t x, byte y, uint32_t color);
  byte charWidth(char c);
  int16_t textWidth(const String& text);
  byte getGlyphRow(char c, byte row);
  byte patternRow(byte row, byte r0, byte r1, byte r2, byte r3, byte r4);
};

#endif
