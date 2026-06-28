#include "pixelMood.h"

void PixelMood::reset(){
  lastupdate = 0;
  animationOffset = 0;
}

void PixelMood::setPreset(byte preset){
  currentPreset = preset % MOOD_PRESET_COUNT;
  lastupdate = 0;
}

byte PixelMood::getPreset(){
  return currentPreset;
}

void PixelMood::nextPreset(){
  setPreset(currentPreset + 1);
}

void PixelMood::previousPreset(){
  if(currentPreset == 0){
    setPreset(MOOD_PRESET_COUNT - 1);
  }else{
    setPreset(currentPreset - 1);
  }
}

void PixelMood::update(unsigned long currentMillis){
  unsigned long interval = currentPreset >= 10 ? 120 : 1000;
  if(currentMillis - lastupdate <= interval){
    return;
  }

  lastupdate = currentMillis;
  animationOffset++;

  switch(currentPreset){
    case 0: renderSolid(255, 255, 255); break;
    case 1: renderSolid(255, 180, 90); break;
    case 2: renderSolid(255, 0, 0); break;
    case 3: renderSolid(255, 80, 0); break;
    case 4: renderSolid(255, 255, 0); break;
    case 5: renderSolid(0, 255, 0); break;
    case 6: renderSolid(0, 255, 255); break;
    case 7: renderSolid(0, 0, 255); break;
    case 8: renderSolid(140, 0, 255); break;
    case 9: renderSolid(255, 0, 120); break;
    case 10: renderRainbowDiagonal(); break;
    case 11: renderRainbowBlocks(); break;
    case 12: renderRandomPixels(); break;
    default: renderRandomSparkle(); break;
  }

  stripPtr->show();
}

void PixelMood::renderSolid(byte red, byte green, byte blue){
  uint32_t color = stripPtr->Color(red, green, blue);
  for(int i = 0; i < BOARDSIZE; ++i){
    stripPtr->setPixelColor(i, color);
  }
}

void PixelMood::renderRainbowDiagonal(){
  for(byte y = 0; y < BOARDHEIGHT; ++y){
    for(byte x = 0; x < BOARDWIDTH; ++x){
      byte colorIndex = (x * 10 + y * 10 + animationOffset * 3) & 0xFF;
      stripPtr->setPixelColor(getPixelIndex(x, y), wheel(colorIndex));
    }
  }
}

void PixelMood::renderRainbowBlocks(){
  for(byte y = 0; y < BOARDHEIGHT; ++y){
    for(byte x = 0; x < BOARDWIDTH; ++x){
      byte colorIndex = ((x / 4) * 45 + (y / 4) * 30 + animationOffset * 2) & 0xFF;
      stripPtr->setPixelColor(getPixelIndex(x, y), wheel(colorIndex));
    }
  }
}

void PixelMood::renderRandomPixels(){
  for(int i = 0; i < BOARDSIZE; ++i){
    stripPtr->setPixelColor(i, wheel((byte)random(0, 255)));
  }
}

void PixelMood::renderRandomSparkle(){
  for(int i = 0; i < BOARDSIZE; ++i){
    if(random(0, 100) < 30){
      stripPtr->setPixelColor(i, wheel((byte)random(0, 255)));
    }
  }
}

uint32_t PixelMood::wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return stripPtr->Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if(wheelPos < 170) {
    wheelPos -= 85;
    return stripPtr->Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return stripPtr->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}
