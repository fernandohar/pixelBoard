#include "pixelArt.h"

void PixelArt::update(unsigned long currentMillis){
	if((currentMillis - lastupdate) > displaySpeed){
		lastupdate = currentMillis;


    //Prevent "backward" movement press
    if(pixelBoardController->getStickyBtnStatus(UP)){
     // moveDir = UP;
    }else if (pixelBoardController->getStickyBtnStatus(DOWN) ){
    //  moveDir = DOWN;
    }else if (pixelBoardController->getStickyBtnStatus(LEFT)){
      PreviousFolder();
    }else if (pixelBoardController->getStickyBtnStatus(RIGHT) ){
    // moveDir = RIGHT;
      NextFolder();
    }
    pixelBoardController->clearStickyBtns();

    
		if(isTraverseFolderMode){
			traverseFolder();
		}else{
			openFolder(currentFolderPointer);
		}

   
		playFileInFolder();
   
	}
}

void PixelArt::reset(){
	rootFolderCount = 0;
	currentFolderPointer = -1;
    currentFileIndex = 1;
}

void PixelArt::init(){
	reset();
	//Working directory rewind
	sdPtr->vwd()->rewind();
	
	//All animation files are under the "animation" folder 
	sdPtr->chdir("/animation");
	while(file.openNext(sdPtr->vwd(), O_READ)){
	  if(!file.isHidden() && file.isDir()){
        directoryIndex[rootFolderCount++] = file.dirIndex();
	  }
      file.close();  
	}
	
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t PixelArt::read16(SdFile& f) {
	uint16_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read(); // MSB
	return result;
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint32_t PixelArt::read32(SdFile& f) {
	uint32_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read();
	((uint8_t *)&result)[2] = f.read();
	((uint8_t *)&result)[3] = f.read(); // MSB
	return result;
} 

void PixelArt::NextFolder(){
  if(++currentFolderPointer >= rootFolderCount){
      currentFolderPointer = 0;
    }
  openFolder(currentFolderPointer);
}

void PixelArt::PreviousFolder(){
  if(--currentFolderPointer <0){
      currentFolderPointer = rootFolderCount - 1;
    }
    openFolder(currentFolderPointer);

}

void PixelArt::openFolder(byte folderIndex){
	sdPtr->chdir("/animation");
	file.open(sdPtr->vwd(), directoryIndex[folderIndex], O_READ);
    file.getName(folderName, 20);
    sdPtr->chdir(folderName);
    file.close();
}

void PixelArt::traverseFolder(){
  if(currentFolderCompleted){
    if(++currentFolderPointer >= rootFolderCount){
      currentFolderPointer = 0;
    }
	  openFolder(currentFolderPointer);
  }
  currentFolderCompleted = false; 
}

void PixelArt::playFileInFolder(){
  char bmpFileName[9] = ""; // 3-digit number + .bmp + null byte
  char tempPost[3] = "";
  for(int j=0; j < 2 - log10(currentFileIndex); j++){
    strcat(bmpFileName, "0");
  }
  itoa(currentFileIndex, tempPost, 10);
  strcat(bmpFileName, tempPost);
  strcat(bmpFileName, ".bmp");
  
  if (sdPtr->exists(bmpFileName)){      
    bmpDraw(bmpFileName);
    //delay(displaySpeed);
  }else{
    currentFolderCompleted = true;
    currentFileIndex = 1;
  }
  currentFileIndex++;  
}

void PixelArt::displayIcon(char* filename){
 // Serial.println("function: displayIcon");
  sdPtr->chdir("/");
  sdPtr->vwd()->rewind();
  if(sdPtr->exists("/icon")){
    //Serial.println("directory /icon exists");
    sdPtr->chdir("/icon");
    //Serial.println("directory changed");
    //Serial.printf("bmpDraw [%s]\n", filename);
    bmpDraw(filename);
  }else{
    Serial.println("directory icon does not exists");
  }
  
  
	
}
void PixelArt::bmpDraw(char* filename){
  int  bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t  rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int  w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;
  const uint8_t  gridWidth = 16;
  const uint8_t  gridHeight = 16;

  if (!file.isOpen()){
    // Open requested file on SD card
    if (!file.open(filename, O_READ)) {
      //Serial.println(F("File open failed"));
      return;
    }
  }else{
    file.rewind();
  }

  //read16 = read 16 bits (2byte)
  //BMP file header:
  //2byte header-field identifier (A)
  //4byte size of bmp (B)
  //2byte reserved (C)
  //2byte reserved (D)
  //4byte Starting address of the byte where bitmap data can be found (E)
  
  // Parse BMP header
  if(read16(file) == 0x4D42) { // BMP signature (A)
    
    (void)read32(file); // Read & ignore creator bytes (B)
    (void)read32(file); // Read & ignore creator bytes (C) (D)
    bmpImageoffset = read32(file); // Start of image data  (E)
    //Bitmap Information header (DIB):
    //4byte size of DIB (F)
    //2byte size of bmp in pixel (unsigned 16bit) (G)
    //2byte size of bmp in pixel (H)
    //2byte number of color planes, must be 1 (I)
    //2byte number of bits per pixel (J)
    
    // Read DIB header
    //Serial.print(F("DIB Header size: ")); 
    //Serial.println(read32(file));// (F)
    (void)read32(file); // Read & ignore creator bytes (F)
    bmpWidth  = read32(file); //(G)
    bmpHeight = read32(file);//(H)
    if(read16(file) == 1) { // # planes -- must be '1' (I)
      bmpDepth = read16(file); // bits per pixel (J)
      if(bmpDepth == 24) {
        if(read32(file) == 0){ // 0 = uncompressed
          // BMP rows are padded (if needed) to 4-byte boundary
          rowSize = (bmpWidth * 3 + 3) & ~3;
          // If bmpHeight is negative, image is in top-down order.
          // This is not canon but has been observed in the wild.
          if(bmpHeight < 0) {
            bmpHeight = -bmpHeight;
            flip      = false;
          }
          
          // initialize our pixel index
          byte index = 0; // a byte is perfect for a 16x16 grid
          
          // Crop area to be loaded
          //for (row=0; row < bmpHeight; row++) { // For each scanline...
		  for(row = 0; row < BOARDHEIGHT; row++){
            //Serial.printf("draw for row#( %d )\n", row);
            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            
            if(flip){ // Bitmap is stored bottom-to-top order (normal BMP)
              pos = (bmpImageoffset + (bmpHeight - 1 - row) * rowSize);
            }else{     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + row * rowSize;
            }
            if(file.curPosition() != pos) { // Need seek?
              file.seekSet(pos);
              buffidx = sizeof(sdbuffer); // Force buffer reload
            }
            
            for (col=0; col<bmpWidth; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sizeof(sdbuffer)) { // Indeed
                file.read(sdbuffer, sizeof(sdbuffer));
                buffidx = 0; // Set index to beginning
              }
              
              // push to LED buffer 
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];

              // offsetY is beyond bmpHeight
			  if(col >= BOARDWIDTH){
				continue;
			  }
			  //Serial.printf("setPixelColor( %d )\n", getPixelIndex(col, row));
              stripPtr->setPixelColor(getPixelIndex(col, row), Adafruit_NeoPixel::Color(r, g, b)); // paint pixel color
            } // end pixel
          } // end scanline
        }//file not compressed
		else{
			Serial.println("ERROR, BMP must not be compressed");
		}
      } //if bit depth = 24
	  else{
		Serial.println("ERROR, BMP depth must be 24 bit");
	  }
    } //if planes = 1
  }//if 0x4D42
  
  stripPtr->show();
  // NOTE: strip.show() halts all interrupts, including the system clock.
  // Each call results in about 6825 microseconds lost to the void.
  file.close();
}
