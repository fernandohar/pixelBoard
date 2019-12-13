#include "gameSnake.h"

void GameSnake:: update(unsigned long currentMillis){
	if((currentMillis - lastupdate) > gameSpeed){
		lastupdate = currentMillis;
		
	
		
		//Prevent "backward" movement press
		if(pixelBoardController->getStickyBtnStatus(UP) && moveDir != DOWN){
			moveDir = UP;
		}else if (pixelBoardController->getStickyBtnStatus(DOWN) && moveDir != UP){
		  moveDir = DOWN;
		}else if (pixelBoardController->getStickyBtnStatus(LEFT) && moveDir != RIGHT){
			moveDir = LEFT;
		}else if (pixelBoardController->getStickyBtnStatus(RIGHT) && moveDir  != LEFT){
			moveDir = RIGHT;
		}
		pixelBoardController->clearStickyBtns();
		//Clean up Key-Press 
		//keyUP = keyDOWN =  keyLEFT = keyRIGHT = false;

    /*if(keyUP && moveDir != DOWN){
      moveDir = UP;
      }else if (keyDOWN && moveDir != UP){
      moveDir = DOWN;
      }else if (keyLEFT && moveDir != RIGHT){
      moveDir = LEFT;
      }else if (keyRIGHT && moveDir  != LEFT){
      moveDir = RIGHT;
    }*/
    keyUP = keyDOWN =  keyLEFT = keyRIGHT = false;
		//
		byte futurePosition = getRelativePos( snake[headPointer],  moveDir);
		
		//Collision checking
		if(collideWithSnake(futurePosition)){
			gameOver();
			return;
		}
		snake[getNextHeadPointer()] = futurePosition;
		
		//Update Head and Tail Pointer
		advanceHeadPointer();
		//Check if fruit is eaten
		if(futurePosition == fruitPosition){
			fruitPosition = -1; //Invalidate old Fruit
			snakeLength++;
			gameSpeed = max(0, gameSpeed - 10);
			}else{
			advanceTailPointer();
		}
		
		//CLEAR SCREEN
		for(short i = 0; i < BOARDSIZE; ++i ){
			stripPtr->setPixelColor(i, BLACK);
		}
		short ptr = tailPointer;
		for(short i = 0; i < snakeLength; ++i ){
			stripPtr->setPixelColor(getPixelIndex(snake[ptr++]), RED);
			if(ptr >= BOARDSIZE){
				ptr = 0;
			}
		}
		//Paint the Snake head blue
		stripPtr->setPixelColor(getPixelIndex(snake[headPointer]), BLUE);
		
		if(fruitPosition < 0){
			fruitPosition = genFruitPosition(); //Make a new fruit
		}
		//Display Fruit
		stripPtr->setPixelColor(getPixelIndex(fruitPosition), GREEN);
		stripPtr->show();
	}
}

void GameSnake::gameOver(){
	for(short i=0;i<BOARDSIZE;i++){
		stripPtr->setPixelColor(i, RED);
	}
	stripPtr->show();
	
	delay(2000);
	reset();
}

void GameSnake::reset(){
	snakeLength = initialSnakeLength;
	moveDir = RIGHT;
	gameSpeed = INITIALSNAKESPEED;
	
	headPointer = initialSnakeLength - 1;
	tailPointer = 0;

	//Clear the Snake 
	//short boardSize = BOARDWIDTH * BOARDHEIGHT;
	for(short i = 0; i < BOARDSIZE ; ++i ){
		snake[i] = -1;
	}
	//Initialize the Snake Position
	snake[0] = BOARDSIZE / 2 ;//Tail
	for(short i = 1; i < initialSnakeLength; ++i){
		snake[i] = getRelativePos(snake[i - 1], RIGHT);
	}
	
	fruitPosition = genFruitPosition();
}

short GameSnake::genFruitPosition(){
	short temp = -1;
	while( temp < 0 || collideWithSnake(temp)){
		temp = rand() % BOARDSIZE;
	}
	return temp;
}

bool GameSnake::collideWithSnake(short checkPosition){
	short pos = tailPointer;
	for(short i = 0; i < snakeLength; ++i ){
		if(snake[pos++] == checkPosition){
			return true;
		}
		if(( pos ) >= BOARDSIZE){
			pos = 0;
		}
	}
	return false;
}

byte GameSnake::getRelativePos(byte parentPos, int direction){
	//position starts with 0
	//short boardSize = BOARDWIDTH * BOARDHEIGHT;
	if(direction == UP){
		if(parentPos < BOARDWIDTH){
			return parentPos + BOARDSIZE - BOARDWIDTH;
			}else{
			return parentPos - BOARDWIDTH;
		}
		
		}else if (direction == DOWN){
		if((parentPos + BOARDWIDTH) >= BOARDSIZE){
			return parentPos - BOARDSIZE + BOARDWIDTH;
			}else{
			return parentPos + BOARDWIDTH;
		}
		}else if(direction == RIGHT){
		if (((parentPos + 1) % BOARDWIDTH) == 0){
			return parentPos + 1 - BOARDWIDTH;
			}else{
			return parentPos + 1;
		}
		
		}else if (direction == LEFT){
		if((parentPos % BOARDWIDTH) == 0){
			return parentPos + BOARDWIDTH - 1;
			}else{
			return parentPos - 1;
		}
	}
}
