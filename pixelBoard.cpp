#include "pixelBoard.h"
 void PixelBoard::update(unsigned long currentMillis){
	if((currentMillis - lastupdate) > 1000){
		lastupdate = currentMillis;
		fill(ledFill_R, ledFill_G, ledFill_B, true);
	 }
 }
 
 
 void PixelBoard::reset(){
	ledFill_R = 0;
	ledFill_G = 0;
	ledFill_B = 0;	 
	 
}

void PixelBoard::fill(byte red, byte green, byte blue , bool saveColor){
  if(saveColor){
	ledFill_R = red;
	ledFill_G = green;
	ledFill_B = blue;	
  }
  uint32_t color = stripPtr->Color(red, green, blue);
  
  for(int i=0; i < BOARDSIZE; i++){
    stripPtr->setPixelColor(i, color); // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  }
  stripPtr->show();

}