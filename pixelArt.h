//pixelArt.h
//Class to display pixel art using BMP from SD card
#ifndef _PIXELART_h
#define _PIXELART_h
#include "SdFat.h"
#include "PixelBoardBase.h"
#include "PixelBoardController.h"
#include <algorithm>    /* std::max */

using namespace sdfat;
//#include "SdFat/src/SdFat.h" //https://github.com/greiman/SdFat
//#include <SdFat.h> //https://github.com/greiman/SdFat
#define BUFFPIXEL 1
class PixelArt : public PixelBoardBase{
public:
	PixelArt(){};
	PixelArt(Adafruit_NeoPixel* stripPtr, SdFat* s, PixelBoardController* pixelBoardControllerPtr) : PixelBoardBase(stripPtr), sdPtr(s), pixelBoardController(pixelBoardControllerPtr){};
	~PixelArt(){};
	
	int currentFolderPointer = -1;
	int displaySpeed = 200;
	int rootFolderCount = 0;
	
	
	void update(unsigned long currentMillis);
	void reset();
	void init();
	
	void faster(){
		displaySpeed =  max(0, displaySpeed - 50);
	}
	void slower(){
		displaySpeed =  displaySpeed + 50;
	}
	void traverseFolder();
	void playFileInFolder();
	void openFolder(byte folderIndex);
	

	void startTraverseFolders(){
		this->isTraverseFolderMode = true;
	}
	
	void stopTraverseFolders(){
		this->isTraverseFolderMode = false;
	}
	
	void displayIcon(char* filename);
private:
  PixelBoardController* pixelBoardController;
  
	SdFat* sdPtr;
	SdFile file;

	char folderName[20];
	char fileName[20];
	byte directoryIndex[50];
	
	
	// These read 16- and 32-bit types from the SD card file.
	// BMP data is stored little-endian, Arduino is little-endian too.
	// May need to reverse subscript order if porting elsewhere.
	uint16_t read16(SdFile& f);
	uint32_t read32(SdFile& f);
	  
	
	void NextFolder();
  void PreviousFolder();
	
	void bmpDraw(char* filename);
	int  currentFileIndex = 1;
	bool currentFolderCompleted = true;
	bool isTraverseFolderMode = false;
};

#endif 
