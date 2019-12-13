//pixelClock.cpp
#include "pixelClock.h"

PixelClock:: ~PixelClock(){
	
}
void PixelClock::reset(){};
void PixelClock::update(unsigned long currentMillis){
  if((currentMillis - lastupdate) > 999){
		lastupdate = currentMillis;
		
	for(int i = 0; i < 256; ++i){
      stripPtr->setPixelColor( i, BLACK);
    }
	
	const RtcDateTime dt = rtc->GetDateTime();
	
 
	 uint8_t hour = dt.Hour();
    uint8_t minute = dt.Minute();
    
    byte tempDigit1  = hour / 10;
    byte tempDigit2 = hour % 10;
    byte tempDigit3 = minute / 10;
    byte tempDigit4 = minute %10;
    uint8_t tempSecond = dt.Second();
    
    if (tempDigit4 != digit4){
      digit1 = tempDigit1;
      digit1Color = getRandomColor(); 
      digit2 = tempDigit2;
      digit2Color = getRandomColor(); 
      digit3 = tempDigit3;
      digit3Color = getRandomColor(); 
      
      digit4 = tempDigit4;
      digit4Color = getRandomColor(); 
    }
    
    displayDigit(digit1, 0, 3, digit1Color);
    displayDigit(digit2, 4, 3, digit2Color);
    displayDigit(digit3, 9, 3, digit3Color);
    displayDigit(digit4, 13, 3, digit4Color);
    
    if (tempSecond != second){
      second = tempSecond;
      secondColor = getRandomColor();
    }
    
    displaySecond(second, secondColor);
    
    stripPtr->show();
  }
}

void PixelClock::displayDigit(byte digit, byte offsetX, byte offsetY, uint32_t color){ //must call strip.show() else where to display the digit
  byte pixelIndex;
  for(int x = 0; x < 3; x++){
    for(int y = 0; y < 10; y++){
      if(bitRead(DIGIT_FONT1[x][y], 15 - digit)){
        pixelIndex = getPixelIndex(x + offsetX, y + offsetY);
        stripPtr->setPixelColor( pixelIndex, color); 
      }
    }
  }
}


void PixelClock::displaySecond(byte second, uint32_t color){
  if (second <= 15){
    stripPtr->setPixelColor( second, color); 
    }else if (second <= 30){
    stripPtr->setPixelColor(getPixelIndex(15, second - 15), color);
    }else if (second <= 45){
    stripPtr->setPixelColor(getPixelIndex( 15 - (second - 30), 15) , color);
    }else{
    stripPtr->setPixelColor(getPixelIndex(0, 15 - (second - 45)) , color);
  }
}
