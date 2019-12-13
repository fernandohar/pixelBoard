//pixelBoard.h
//The BASE class for all modules of pixelBoard
//Provide function to mapping coordinates to pixelBoard's ZigZag coordinate
#ifndef _PIXELBOARD_h
#define _PIXELBOARD_h
#include "PixelBoardBase.h"

class PixelBoard : public PixelBoardBase{
public:
  PixelBoard(){};
  PixelBoard(Adafruit_NeoPixel* stripPtr) : PixelBoardBase(stripPtr){ };
  ~PixelBoard(){};
  
  void update(unsigned long currentMillis);
  void reset();
  void fill(byte red, byte green, byte blue , bool saveColor);
  byte ledFill_R = 0;
  byte ledFill_G = 0;
  byte ledFill_B = 0;
};

#endif