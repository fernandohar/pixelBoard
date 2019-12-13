//Original Game Copied from https://github.com/scout119/RGB123/tree/master/Tetris
//
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
#ifndef _GAME_TETRIS_H
#define _GAME_TETRIS_H
#include "PixelBoardBase.h"
#include "PixelBoardController.h"

#define LED_ROWS 16
#define LED_COLUMNS 16
#define GAME_COLUMNS 10
#define GAME_ROWS 16

#include "GamePiece.h"
class GameTetris : public PixelBoardBase{
public:
GameTetris(){}
GameTetris(Adafruit_NeoPixel* stripPtr, PixelBoardController* pixelBoardControllerPtr): PixelBoardBase(stripPtr), pixelBoardController(pixelBoardControllerPtr){};
~GameTetris(){};








void render();
void readButtons();
	void update(unsigned long currentMillis);
	void reset();

private: 
    PixelBoardController* pixelBoardController;
	byte gameField[GAME_ROWS * GAME_COLUMNS];
	byte p1[4] = { 1, 1, 1, 1};  
	byte p2[6] = { 2, 2, 2, 0, 2, 0};
	byte p3[6] = { 3, 0, 3, 0, 3, 3};
	byte p4[6] = { 0, 4, 0, 4, 4, 4};
	byte p5[6] = { 5, 5, 0, 0, 5, 5};
	byte p6[6] = { 0, 6, 6, 6, 6, 0};
	byte p7[4] = { 7, 7, 7, 7 };
	GamePiece  _gamePieces[7] = 
	{
		GamePiece(2, 2, p1 ),
		GamePiece(3, 2, p2 ),
		GamePiece(3, 2, p3 ),
		GamePiece(2, 3, p4 ),
		GamePiece(2, 3, p5 ),
		GamePiece(2, 3, p6 ),
		GamePiece(4, 1, p7 )
	};
	unsigned long colors[7] =
	{
	RED,
	BLUE,
	GREEN,
	YELLOW,
	MAGENTA,
	SALMON,
	INDIGO  
	};
	GamePiece * fallingPiece = NULL;
GamePiece * nextPiece = NULL;
byte gameLevel = 1;
byte currentRow = 0;
byte currentColumn = 0;
byte gameLines = 0;
boolean gameOver = false;
void clearLEDs();
void setColor( int row, int column, unsigned long color);
void startGame();
void newLevel(uint8_t level);
void emptyField();
void newPiece();
boolean isValidLocation(GamePiece & piece, byte column, byte row);
void moveDown();
void drop();
void updateScore();
void rotateLeft();
void rotateRight();
void moveRight();
void moveLeft();

};

#endif
