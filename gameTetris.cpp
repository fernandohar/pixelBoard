//gameTetris.cpp
//Original game copied from https://github.com/scout119/RGB123/tree/master/Tetris
//Copyright (C) 2013 Valentin Ivanov http://www.breakcontinue.com.
//
//This work is licensed under a Creative Commons 
//Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//See http://creativecommons.org/licenses/by-nc-sa/3.0/
//
//All other rights reserved.
//
//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
//EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
//WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
#include "gameTetris.h"

void GameTetris::update(unsigned long currentMillis){

	readButtons();
	//check if enough time passed to move the piece
	//with each level this time is geting shorter
	if( currentMillis - lastupdate > (800 / (gameLevel * 0.80)) )
	{
		if( !gameOver )
		{        
			moveDown();
			gameOver = !isValidLocation(*fallingPiece, currentColumn, currentRow);       
		}else{
    
      startGame();
		}

   
		lastupdate = currentMillis;

		
	}
	render();
}
void GameTetris::reset(){
	clearLEDs();
	startGame();
	stripPtr->show();
}

void GameTetris::clearLEDs()
{
	for (int i=0; i<BOARDSIZE; i++)
	{
		stripPtr->setPixelColor(i, 0);
	}
}

void GameTetris::readButtons(){
	if(pixelBoardController->getStickyBtnStatus(BTNA)){
		if( gameOver )
		{
			startGame();
			return;
		}
		else
			rotateLeft();
	}else if (pixelBoardController->getStickyBtnStatus(BTNB)){
	  if( gameOver )
		{
			startGame();
			return;
		}
		else
			rotateRight();
	}else if (pixelBoardController->getStickyBtnStatus(LEFT)){
		moveLeft();
	}else if (pixelBoardController->getStickyBtnStatus(RIGHT)){
		moveRight();
	}else if (pixelBoardController->getStickyBtnStatus(DOWN)){
		if( gameOver )
		{
			startGame();
			return;
		}
		else
			drop();
	}
	pixelBoardController->clearStickyBtns();

}

/// <summary>
/// Conversion from traditional coordinats into matrix's zig zag order
/// </summary>
void GameTetris::setColor( int row, int column, unsigned long color)
{
	int led = getPixelIndex(column, row);
	//int odd = row%2;
	//int led = 0;

//	if( !odd )
//		led = (row<<4)+column; 
//	else
//		led= ((row+1)<<4)-1 - column;

	stripPtr->setPixelColor(led,color);
}

void GameTetris::render(){
	int value = 0;
	unsigned long color = 0;

	//render game field first
	for( int row = 0; row < GAME_ROWS; row++)
	{
		for( int col = 0; col < GAME_COLUMNS; col++)
		{
			color = 0;
			value = gameField[row * GAME_COLUMNS + col];
			if( value > 0)
				color = colors[value-1];
			setColor(row,col, color);
		}
	}

	//render falling piece
	for( int row = 0; row < fallingPiece->Rows; row++)
	{
		for( int col = 0; col < fallingPiece->Columns; col++)
		{
			value = (*fallingPiece)(row,col);
			if( value > 0)    
				setColor(currentRow+row,currentColumn+col, colors[value-1]);
		}
	}

	//render divider line
	for( int row = 0; row < LED_ROWS; row++)
	{
		for( int col = GAME_COLUMNS; col < LED_COLUMNS; col ++)
		{
			if( col == GAME_COLUMNS )
				setColor(row,col, WHITE);
			else
				setColor(row,col, 0);
		}
	}

	//render next piece
	for( int row = 0; row < nextPiece->Rows; row++)
	{
		for( int col = 0; col < nextPiece->Columns; col++)
		{
			value = (*nextPiece)(row,col);
			if( value > 0)    
				setColor(7+row,12+col, colors[value-1]);
			else
				setColor(7+row,12+col, 0);
		}
	}  
	stripPtr->show();
};
void GameTetris::startGame(){
	
    //Serial.println("Start game");
	nextPiece=NULL;
	gameLines = 0;
	lastupdate = 0;
	newLevel(1);
	gameOver = false;
	render();
};
void GameTetris::newLevel(uint8_t level){
	gameLevel = level;
	emptyField();
	newPiece();
};

void GameTetris::emptyField(){
	for(int i = 0; i < GAME_ROWS * GAME_COLUMNS; i++ )
	{
		gameField[i] = 0;
	}
};

void GameTetris::newPiece(){
	int next;

	currentColumn = 4;
	currentRow = 0;


	if (nextPiece == NULL)
	{
		next = random(100) % 7;  
		nextPiece = &_gamePieces[next];
	}

	if( fallingPiece != NULL )
		delete fallingPiece;

	fallingPiece = new GamePiece(*nextPiece);

	next = random(100) % 7;
	nextPiece = &_gamePieces[next];  
};


boolean GameTetris::isValidLocation(GamePiece & piece, byte column, byte row){
	for (int i = 0; i < piece.Rows; i++)
		for (int j = 0; j < piece.Columns; j++)
		{
			int newRow = i + row;
			int newColumn = j + column;                    

			//location is outside of the fieled
			if (newColumn < 0 || newColumn > GAME_COLUMNS - 1 || newRow < 0 || newRow > GAME_ROWS - 1)
			{
				//piece part in that location has a valid square - not good
				if (piece(i, j) != 0)
					return false;
			}else
			{
				//location is in the field but is already taken, pice part for that location has non-empty square 
				if (gameField[newRow*GAME_COLUMNS + newColumn] != 0 && piece(i, j) != 0)
					return false;
			}
		}

		return true; 
};

void GameTetris::moveDown(){
	if (isValidLocation(*fallingPiece, currentColumn, currentRow + 1))
	{
		currentRow +=1;
		return;
	}


	//The piece can't be moved anymore, merge it into the game field
	for (int i = 0; i < fallingPiece->Rows; i++)
	{
		for (int j = 0; j < fallingPiece->Columns; j++)
		{
			byte value = (*fallingPiece)(i, j);
			if (value != 0)
				gameField[(i + currentRow) * GAME_COLUMNS + (j + currentColumn)] = value;
		}
	}

	//Piece is merged update the score and get a new pice
	updateScore();            
	newPiece();  
};
void GameTetris::drop(){
	while (isValidLocation(*fallingPiece, currentColumn, currentRow + 1))
		moveDown();
};

void GameTetris::moveLeft(){
		if (isValidLocation(*fallingPiece, currentColumn - 1, currentRow))
		currentColumn--;
};


void GameTetris::moveRight(){
		if (isValidLocation(*fallingPiece, currentColumn + 1, currentRow))
		currentColumn++;
};

void GameTetris::rotateRight(){
	GamePiece * rotated = fallingPiece->rotateRight();

	if (isValidLocation(*rotated, currentColumn, currentRow))
	{
		delete fallingPiece;
		fallingPiece = rotated;
	}else
        {
          delete rotated;
        }
};
void GameTetris::updateScore(){
	int count = 0;
	for (int row = 1; row < GAME_ROWS; row++)
	{
		boolean goodLine = true;
		for (int col = 0; col < GAME_COLUMNS; col++)
		{
			if (gameField[row *GAME_COLUMNS + col] == 0)
				goodLine = false;
		}

		if (goodLine)
		{
			count++;
			for (int i = row; i > 0; i--)
				for (int j = 0; j < GAME_COLUMNS; j++)
					gameField[i *GAME_COLUMNS +j] = gameField[(i - 1)*GAME_COLUMNS+ j];
		}
	}


	if (count > 0)
	{
		//_gameScore += count * (_gameLevel * 10);
		gameLines += count;


		int nextLevel = (gameLines / GAME_ROWS) + 1;
		if (nextLevel > gameLevel)
		{
			gameLevel = nextLevel;
			newLevel(gameLevel);
		}
	}
}
void GameTetris::rotateLeft(){
		GamePiece * rotated = fallingPiece->rotateLeft();

	if (isValidLocation(*rotated, currentColumn, currentRow))
	{
		delete fallingPiece;
		fallingPiece = rotated;
	}else
        {
          delete rotated;
        }
};
