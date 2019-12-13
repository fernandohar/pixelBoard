//GameSnake.h
#ifndef _GAMESNAKE_h
#define _GAMESNAKE_h

#include "PixelBoardBase.h"
#include "PixelBoardController.h"
#include <cstdlib>      /* for std::rand() and std::srand() */
#include <time.h>       /* time */
#include <algorithm>    /* std::max */


#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3



#define INITIALSNAKESPEED 300
class GameSnake : public PixelBoardBase{
public:
  //update function also update the game logic and NeoPixel board
  void update(unsigned long currentMillis);
  
  void reset();
  GameSnake(){};
  GameSnake(Adafruit_NeoPixel* stripPtr, short length, PixelBoardController* pixelBoardControllerPtr) : PixelBoardBase(stripPtr), initialSnakeLength(length), pixelBoardController(pixelBoardControllerPtr){
	std::srand(time(NULL));
    reset();	
  }
  
  ~GameSnake(){};
  
  void gameOver();
  
  //short* getSnake(){ return snake;}
  
  bool getKeyUP(){return keyUP;}
  void setKeyUP(bool value){keyUP = value;}
  
  bool getKeyDOWN(){return keyDOWN;}
  void setKeyDOWN(bool value){keyDOWN = value;}
  
  bool getKeyLEFT(){return keyLEFT;}
  void setKeyLEFT(bool value){keyLEFT = value;}
  
  bool getKeyRIGHT(){return keyRIGHT;}
  void setKeyRIGHT(bool value){keyRIGHT = value;}
  
private:
  PixelBoardController* pixelBoardController;
  bool keyUP = false;
  bool keyDOWN = false;
  bool keyLEFT = false;
  bool keyRIGHT = false; 
  
  //one-line coordinate to save the snake position.
  //Snake position is saved with circular array, with headPosition and tailPosition to indicate the head and tail.
  //Current implentation is not 
  short snake[BOARDSIZE]; // byte is 0 to 255 / short is -32,768 to 32,767
  short initialSnakeLength;
  
  short fruitPosition;

  //to keep track of current snake status
  short headPointer;
  short tailPointer;
  short snakeLength; 
  
  byte moveDir; //0: UP, 1: RIGHT, 2: DOWN, 3: LEFT
  

  short gameSpeed; //Control the Speed of update(), thus make the game faster / slower. Smaller value = faster
  
  //Private functions
  bool collideWithSnake(short checkPosition);
  
  //Pass in direction(UP/RIGHT/DOWN/LEFT), this function returns the pixel WRT the parentPos, in One-line coordinate
  byte getRelativePos(byte parentPos, int direction);
  
  byte getNextHeadPointer(){
    return ((headPointer + 1) < BOARDSIZE)? (headPointer + 1) : 0;
  }
  
  void advanceHeadPointer(){
    headPointer = ((headPointer + 1) < BOARDSIZE)? (headPointer + 1) : 0;
  }
  
  void advanceTailPointer(){
    tailPointer = ((tailPointer + 1) < BOARDSIZE)? (tailPointer + 1) : 0;
  }

  short genFruitPosition(); //function to generate new Fruit position

};

#endif
