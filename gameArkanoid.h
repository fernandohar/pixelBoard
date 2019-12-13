#ifndef _GAME_ARKANOID_H
#define _GAME_ARKANOID_H

#include "PixelBoardBase.h"
#include "PixelBoardController.h"
#include <cstdlib>      /* for std::rand() and std::srand() */
#include <algorithm>    /* std::max */
#define COL_OF_BRICKS 8
#define ROWS_OF_BRICKS 6
class Brick{
public:
Brick(uint8_t cornerX, uint8_t cornerY, unsigned long brickColor) : 
	x(cornerX), y(cornerY), color(brickColor){
	active = true;
}
bool active;
uint8_t x;
uint8_t y;
unsigned long color;
};


class BrickArray{	
public:
BrickArray(){
	reset();
}
Brick* getBrick(int x, int y){
	return brickPS[x][y];
}

Brick* brickPS[ROWS_OF_BRICKS][COL_OF_BRICKS];
void reset(){
  int i = 0;
  for(int r = 0 ; r < ROWS_OF_BRICKS; r++){
    for(int c = 0 ; c < COL_OF_BRICKS ; c++){
      brickPS[r][c] = new Brick(c * 2, r,  colors[c ]);
    }
  }
}
private:
unsigned long colors[8] =
	{
	RED,
	BLUE,
	GREEN,
	YELLOW,
	MAGENTA,
	SALMON,
	INDIGO,
	ORANGERED
	};
};

class Paddle{
public:
  Paddle(){
	reset();
  };
  bool moveLeft(){
    byte positionXOld = positionX;
    positionX= max(0, positionX - 1);
    return positionXOld != positionX;
  }
  bool moveRight(){
    byte positionXOld = positionX;
    positionX = min(BOARDWIDTH - width, positionX + 1);
    return positionXOld != positionX;
  }
  void reset(){
    width = 3;
     positionX = (BOARDWIDTH / 2) - 1;
     positionY = BOARDHEIGHT - 1;  
    
  }
  byte positionX;
  byte width;
  byte positionY;
};

//class Ball : PixelBoardBase{
class Ball{	
public:
	Ball(){
		reset();
	}

   int moveBall(Paddle* paddle){
		x = x += vX;
	    y = y += vY;
		x = min(BOARDWIDTH -1, max(0, x));
		y = min(BOARDHEIGHT - 1, max(0, y));
		
		if(x >= BOARDWIDTH - 1 || x <= 0) { //bounce from two side
			vX = vX * -1;
		}
	  
		if(y <= 0){ //bounce back from top edge
			vY = vY * -1;
		}else if(y >= BOARDHEIGHT -2 && vY > 0){
		    //check if bounce on paddle
			if(x >= paddle->positionX && x <= (paddle->positionX) + paddle->width){
				vY = vY * -1;
			}else{
				reset(); //Stop the ball
				y = BOARDHEIGHT -1; //BALL FALLED TO GROUND
				return -1;
			}
		}
		return 0;
   }

  void reset(){
    vX = 0;
    vY = 0;
  }
  
  void gameStart(){
	vX = -1;
	vY = -1 ;
  }
  void setPos(int x, int y){
	this->x = x;
	this->y = y;
  }

    void setX(int x){
		this->x = x;
	}
	void setY(int y){
		this->y = y;
	}
	int getX(){
		return x;
	}
	int getY(){
		return y;
	}
	int getvX(){
		return vX;
	}
	int getvY(){
		return vY;
	}
	int vX;
	int vY;
private:
	int x;
	int y;

};

class GameArkanoid : public PixelBoardBase{
public:
GameArkanoid(){};

GameArkanoid(Adafruit_NeoPixel* stripPtr, PixelBoardController* pixelBoardControllerPtr): PixelBoardBase(stripPtr), pixelBoardController(pixelBoardControllerPtr){
	paddle = new Paddle();
	ball = new Ball();
	bricks = new BrickArray();
};

~GameArkanoid(){
	paddle = NULL;
	ball = NULL;
	bricks = NULL;
};

void update(unsigned long currentMillis);
void reset();

private:
PixelBoardController* pixelBoardController;
Paddle* paddle;
Ball* ball;
BrickArray* bricks;

int paddlePos = 0;
unsigned long lastGameLogicUpdate = 0;
bool gameStarted = false;

void checkCollisions();
void gameStart();
void debug();
void clearLEDS(){
  for(int i = 0; i < BOARDSIZE; i++){
    stripPtr->setPixelColor(i, 0);
  }
}

};

#endif