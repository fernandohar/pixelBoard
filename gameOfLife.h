#ifndef _GAME_OF_LIFE_H
#define _GAME_OF_LIFE_H

#include "PixelBoardBase.h"
#include "PixelBoardController.h"
#include <cstdlib>      /* for std::rand() and std::srand() */

#define SIZE 8
#define NUMPIXELS 256
#define PIXELSPERROW   16 //Column
#define ROWS   16
class GameOfLife : public PixelBoardBase{
public:
GameOfLife(){};

GameOfLife(Adafruit_NeoPixel* stripPtr, PixelBoardController* pixelBoardControllerPtr): PixelBoardBase(stripPtr), pixelBoardController(pixelBoardControllerPtr){
	
	
};

~GameOfLife(){};

void update(unsigned long currentMillis);
void reset();


private:
PixelBoardController* pixelBoardController;
int pixelLife[NUMPIXELS];
int pixelLifeNext[NUMPIXELS];
int changes = 0;
int changesBefore = 0, count = 0;
unsigned long life = TEAL;
unsigned long dead = BLACK;

int checkNeiberhood(int pixel);
void syncArrays();
void showNextStep();
void refreshNeopixel();
void resetNeopixel();
};

#endif
