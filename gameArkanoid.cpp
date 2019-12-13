#include "gameArkanoid.h"
void GameArkanoid::checkCollisions(){
  int brickLeftEdge, brickRightEdge, brickTopEdge, brickBottomEdge = -1;
  int ballX = ball->getX();
  int ballY = ball->getY();
  
  if(ballY <= ROWS_OF_BRICKS){ //only need to check if it is above the lowest brick
	  for(int row = 0; row < ROWS_OF_BRICKS; row++){
		for(int col = 0; col < COL_OF_BRICKS; col++){
			Brick* brick = bricks->getBrick(row, col);
			if(brick->active){
				brickLeftEdge = brick->x - 1;
				brickRightEdge = brick->x + 2;
				brickTopEdge = brick->y - 1;
				brickBottomEdge = brick->y + 1;
				
				
				if( (ballX == brickLeftEdge || ballX == brickRightEdge) && 
					(ballY == brickTopEdge || ballY == brick->y || ballY == brickBottomEdge )){
					ball->vX *= -1;
					brick->active = false;
					if(ballY == brickBottomEdge){
						ball->vY *= -1;
					}
					return;
				}
				else if( (ballY == brickTopEdge || ballY == brickBottomEdge) && 
						(ballX == brick->x || ballX == brick->x + 1)
				){
					ball->vY *= -1;
					brick->active = false;
					return;
				}		
			}
		}
	  }
  }
}

void GameArkanoid::update(unsigned long currentMillis){

  bool updateScreen = false;
  if((currentMillis - lastupdate) > 50){ //Read input 10 times per seconds
	lastupdate = currentMillis;
   
    if(pixelBoardController->getBtnStatus(LEFT)){
      updateScreen = paddle->moveLeft();
    }else if(pixelBoardController->getBtnStatus(RIGHT)){  
      updateScreen = paddle->moveRight();
    }
	if(pixelBoardController->getStickyBtnStatus(BTNA)){
		gameStart();
	}
	if(pixelBoardController->getStickyBtnStatus(BTNB)){
		reset();
	}
    pixelBoardController->clearStickyBtns();
      clearLEDS();
	  
	  //Update Game Logic
	  if(!gameStarted){
	    //Set the Ball to follow Paddle's middle position
		ball->setPos(
			paddle->positionX + (paddle->width / 2),
			paddle->positionY - 1
		);
	  }else if((currentMillis - lastGameLogicUpdate) > 200){ //game speed = 250
	    lastGameLogicUpdate = currentMillis;
		if (ball->moveBall(paddle) < 0){
			reset();
		}else{
			checkCollisions();
		}
	  }
	  
	  //Render Bricks
  	  for(int r = 0 ; r < ROWS_OF_BRICKS; r++){
		for(int c = 0 ; c < COL_OF_BRICKS ; c++){
		  Brick* brick = bricks->getBrick(r,c);
		  stripPtr->setPixelColor(getPixelIndex(brick->x, brick->y), (brick->active)? brick->color : BLACK);
		  stripPtr->setPixelColor(getPixelIndex((brick->x) + 1, brick->y),  (brick->active)? brick->color : BLACK);
		}
	  } 
	  //Render Ball
      stripPtr->setPixelColor(getPixelIndex(ball->getX(), ball->getY()), WHITE);
	  
	  //Render Paddle
	  for(int i = 0; i < paddle->width; ++i){
		stripPtr->setPixelColor(getPixelIndex(paddle->positionX + i, paddle->positionY), RED);
	  }
	  stripPtr->show();
	}
}
void GameArkanoid::debug(){
	const char FORMAT[] = "checkCollisions ball[%d,%d] ; vBall [%d, %d] ; ";
    char buffer[100];
    snprintf(buffer, 100, FORMAT,  ball->getX(), ball->getY(), ball->getvX(), ball->getvY());
    pixelBoardController->broadcastTXT( buffer);
//	pixelBoardController->broadcastTXT(itoa(ball->x));
}
void GameArkanoid::gameStart(){
	gameStarted = true;
	ball->gameStart();
}
void GameArkanoid::reset(){
  std::srand(time(NULL));
  paddle->reset();
  ball->reset();
  bricks->reset();
  gameStarted = false;
}
