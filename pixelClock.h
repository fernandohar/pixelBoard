//pixelClock.h
//Class to display time with PixelBoardBase
#ifndef _PIXELCLOCK_H
#define _PIXELCLOCK_H
#include "PixelBoardBase.h"
#include <Wire.h>        //I2C device 
#include <RtcDS3231.h>


class PixelClock : public PixelBoardBase{
public:
	PixelClock(){};
	PixelClock(Adafruit_NeoPixel* stripPtr, RtcDS3231<TwoWire>* rtcPtr) : PixelBoardBase(stripPtr), rtc(rtcPtr){};
	~PixelClock();
	
	void update(unsigned long currentMillis);
	void reset();
private:
	byte currentSecondIndicator = 0;
	byte digit1 = 255;
	byte digit2 = 255;
	byte digit3 = 255;
	byte digit4 = 255;
	byte second = 255;

	uint32_t digit1Color, digit2Color, digit3Color, digit4Color, secondColor;
	RtcDS3231<TwoWire>* rtc;
uint16_t DIGIT_FONT1[3][10]={
  { //Column 1
    ((B11111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10001110) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10101110) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10101110) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10101010) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10111110) << 8)+ B10000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11110110) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11110110) << 8)+ B11000000  //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    },{ //X = 1
    ((B11110111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11110111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01000000) << 8)+ B00000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01000000) << 8)+ B00000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01110110) << 8)+ B10000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01110110) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01001000) << 8)+ B01000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B01001000) << 8)+ B00000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11110000) << 8)+ B10000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11110110) << 8)+ B11000000  //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    },{ //X = 2
    ((B10111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10111011) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10111001) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10101111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10101111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10011111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B10011111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11111111) << 8)+ B11000000, //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    ((B11111111) << 8)+ B11000000  //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
  }
};


  uint32_t getRandomColor(){
  return Wheel((byte) random(0, 255)) ;
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return stripPtr->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return stripPtr->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return stripPtr->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

	void displayDigit(byte digit, byte offsetX, byte offsetY, uint32_t color);
	void displaySecond(byte second, uint32_t color);
};
#endif
