#include "gameOfLife.h"

void GameOfLife::update(unsigned long currentMillis){
	if((currentMillis - lastupdate) > 100){
		lastupdate = currentMillis;
		showNextStep();
		 if (changesBefore == changes) {
			count++;
		  } else {
			changesBefore = changes;
			count = 0;
		  }
		  if (count >=  5) {
			resetNeopixel();
			count = 0;
		  }
	}
}

void GameOfLife::reset(){
	std::srand(time(NULL));
	resetNeopixel();
}
void GameOfLife::resetNeopixel() {
  for (int i = 0; i < NUMPIXELS ; i++) {
    int token = random(0, 2);
    if (token == 1) {
      pixelLifeNext[i] = 1;
    } else {
      pixelLifeNext[i] = 0;
    }
  }
  syncArrays();
}
/*
void GameOfLife::render(){
	for (int i=0; i<NUMPIXELS; i++)
	{
		stripPtr->setPixelColor(i, 0);
	}	
}
*/
void GameOfLife::refreshNeopixel() {
  for (int i = 0; i < NUMPIXELS ; i++) {
    if (pixelLife[i] == 1) { //lebendige Zelle
      stripPtr->setPixelColor( i , life);
    } else {
      stripPtr->setPixelColor( i , dead);
    }
  }
  stripPtr->show();
}
void GameOfLife::showNextStep() {

  changes = 0;
  for (int i = 0; i < NUMPIXELS ; i++) {
    int life = checkNeiberhood(i);
    if (life < 2 || life > 3) {
      pixelLifeNext[i] = 0;
      changes++;
    }
    if (life == 3) {
      pixelLifeNext[i] = 1;
      changes++;
    }
  }
  syncArrays();
  refreshNeopixel();
}

void GameOfLife::syncArrays() {
  for (int i = 0; i < NUMPIXELS ; i++) {
    pixelLife[i] = pixelLifeNext[i];
  }
}
int GameOfLife::checkNeiberhood(int pixel) {
  int lifeCount = 0;
  boolean checkLeft = true,
          checkRigth  = true,
          checkBottom  = true,
          checkTop  = true;

  if (PIXELSPERROW - pixel > 0 ) {
    checkLeft = false;
  }
  if (pixel + PIXELSPERROW >= NUMPIXELS) {
    checkRigth = false;
  }
  if (pixel % PIXELSPERROW == 0 ) {
    checkBottom = false;
  }
  if (pixel % PIXELSPERROW == 7 ) {
    checkTop = false;
  }
  if (checkTop && pixelLife[pixel + 1] == 1 ) {
    lifeCount++;
  }
  if (checkBottom && pixelLife[pixel - 1] == 1 ) {
    lifeCount++;
  }
  if (checkLeft  && pixelLife[pixel - PIXELSPERROW] == 1 ) {
    lifeCount++;
  }
  if (checkTop && checkLeft  && pixelLife[pixel - PIXELSPERROW + 1] == 1 ) {
    lifeCount++;
  }
  if (checkBottom && checkLeft && pixelLife[pixel - PIXELSPERROW - 1] == 1 ) {
    lifeCount++;
  }
  if ( checkRigth  &&  pixelLife[pixel + PIXELSPERROW] == 1 ) {
    lifeCount++;
  }
  if (checkTop && checkRigth  &&  pixelLife[pixel + PIXELSPERROW + 1] == 1 ) {
    lifeCount++;
  }
  if (checkBottom && checkRigth && pixelLife[pixel + PIXELSPERROW - 1] == 1 ) {
    lifeCount++;
  }
  return lifeCount;
}
