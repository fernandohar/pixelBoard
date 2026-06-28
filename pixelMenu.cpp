#include "pixelMenu.h"

void PixelMenu::reset(){
  currentText = "";
  scrollOffset = 0;
  lastScroll = 0;
}

void PixelMenu::update(unsigned long currentMillis){
  showText(currentText, currentMillis);
}

void PixelMenu::showText(const String& text, unsigned long currentMillis, uint32_t color){
  if(!text.equals(currentText)){
    currentText = text;
    scrollOffset = 0;
    lastScroll = currentMillis;
  }

  clear();

  int16_t width = textWidth(currentText);
  if(width <= BOARDWIDTH){
    drawText(currentText, (BOARDWIDTH - width) / 2, 5, color);
  }else{
    if(currentMillis - lastScroll > 180){
      lastScroll = currentMillis;
      scrollOffset++;
      if(scrollOffset > width + 4){
        scrollOffset = 0;
      }
    }
    int16_t x = 0 - scrollOffset;
    drawText(currentText, x, 5, color);
    drawText(currentText, x + width + 4, 5, color);
  }

  stripPtr->show();
}

void PixelMenu::clear(){
  for(int i = 0; i < BOARDSIZE; ++i){
    stripPtr->setPixelColor(i, BLACK);
  }
}

void PixelMenu::drawText(const String& text, int16_t x, byte y, uint32_t color){
  int16_t cursorX = x;
  for(unsigned int i = 0; i < text.length(); ++i){
    char c = text.charAt(i);
    drawChar(c, cursorX, y, color);
    cursorX += charWidth(c) + 1;
  }
}

void PixelMenu::drawChar(char c, int16_t x, byte y, uint32_t color){
  byte width = charWidth(c);
  for(byte row = 0; row < 5; ++row){
    byte bits = getGlyphRow(c, row);
    for(byte col = 0; col < width; ++col){
      if(bits & (1 << (width - 1 - col))){
        int16_t px = x + col;
        int16_t py = y + row;
        if(px >= 0 && px < BOARDWIDTH && py >= 0 && py < BOARDHEIGHT){
          stripPtr->setPixelColor(getPixelIndex((byte)px, (byte)py), color);
        }
      }
    }
  }
}

byte PixelMenu::charWidth(char c){
  if(c == ' '){
    return 2;
  }
  if(c == ':' || c == '.' || c == '-' || c == '+' || c == '/'){
    return 1;
  }
  return 3;
}

int16_t PixelMenu::textWidth(const String& text){
  if(text.length() == 0){
    return 0;
  }

  int16_t width = 0;
  for(unsigned int i = 0; i < text.length(); ++i){
    width += charWidth(text.charAt(i));
    if(i < text.length() - 1){
      width += 1;
    }
  }
  return width;
}

byte PixelMenu::patternRow(byte row, byte r0, byte r1, byte r2, byte r3, byte r4){
  switch(row){
    case 0: return r0;
    case 1: return r1;
    case 2: return r2;
    case 3: return r3;
    default: return r4;
  }
}

byte PixelMenu::getGlyphRow(char c, byte row){
  if(c >= 'a' && c <= 'z'){
    c -= 32;
  }

  switch(c){
    case 'A': return patternRow(row, 0b010, 0b101, 0b111, 0b101, 0b101);
    case 'B': return patternRow(row, 0b110, 0b101, 0b110, 0b101, 0b110);
    case 'C': return patternRow(row, 0b011, 0b100, 0b100, 0b100, 0b011);
    case 'D': return patternRow(row, 0b110, 0b101, 0b101, 0b101, 0b110);
    case 'E': return patternRow(row, 0b111, 0b100, 0b110, 0b100, 0b111);
    case 'F': return patternRow(row, 0b111, 0b100, 0b110, 0b100, 0b100);
    case 'G': return patternRow(row, 0b011, 0b100, 0b101, 0b101, 0b011);
    case 'H': return patternRow(row, 0b101, 0b101, 0b111, 0b101, 0b101);
    case 'I': return patternRow(row, 0b111, 0b010, 0b010, 0b010, 0b111);
    case 'J': return patternRow(row, 0b001, 0b001, 0b001, 0b101, 0b010);
    case 'K': return patternRow(row, 0b101, 0b101, 0b110, 0b101, 0b101);
    case 'L': return patternRow(row, 0b100, 0b100, 0b100, 0b100, 0b111);
    case 'M': return patternRow(row, 0b101, 0b111, 0b111, 0b101, 0b101);
    case 'N': return patternRow(row, 0b101, 0b111, 0b111, 0b111, 0b101);
    case 'O': return patternRow(row, 0b010, 0b101, 0b101, 0b101, 0b010);
    case 'P': return patternRow(row, 0b110, 0b101, 0b110, 0b100, 0b100);
    case 'Q': return patternRow(row, 0b010, 0b101, 0b101, 0b111, 0b011);
    case 'R': return patternRow(row, 0b110, 0b101, 0b110, 0b101, 0b101);
    case 'S': return patternRow(row, 0b011, 0b100, 0b010, 0b001, 0b110);
    case 'T': return patternRow(row, 0b111, 0b010, 0b010, 0b010, 0b010);
    case 'U': return patternRow(row, 0b101, 0b101, 0b101, 0b101, 0b111);
    case 'V': return patternRow(row, 0b101, 0b101, 0b101, 0b101, 0b010);
    case 'W': return patternRow(row, 0b101, 0b101, 0b111, 0b111, 0b101);
    case 'X': return patternRow(row, 0b101, 0b101, 0b010, 0b101, 0b101);
    case 'Y': return patternRow(row, 0b101, 0b101, 0b010, 0b010, 0b010);
    case 'Z': return patternRow(row, 0b111, 0b001, 0b010, 0b100, 0b111);
    case '0': return patternRow(row, 0b111, 0b101, 0b101, 0b101, 0b111);
    case '1': return patternRow(row, 0b010, 0b110, 0b010, 0b010, 0b111);
    case '2': return patternRow(row, 0b111, 0b001, 0b111, 0b100, 0b111);
    case '3': return patternRow(row, 0b111, 0b001, 0b111, 0b001, 0b111);
    case '4': return patternRow(row, 0b101, 0b101, 0b111, 0b001, 0b001);
    case '5': return patternRow(row, 0b111, 0b100, 0b111, 0b001, 0b111);
    case '6': return patternRow(row, 0b111, 0b100, 0b111, 0b101, 0b111);
    case '7': return patternRow(row, 0b111, 0b001, 0b010, 0b010, 0b010);
    case '8': return patternRow(row, 0b111, 0b101, 0b111, 0b101, 0b111);
    case '9': return patternRow(row, 0b111, 0b101, 0b111, 0b001, 0b111);
    case ':': return patternRow(row, 0b0, 0b1, 0b0, 0b1, 0b0);
    case '.': return patternRow(row, 0b0, 0b0, 0b0, 0b0, 0b1);
    case '-': return patternRow(row, 0b0, 0b0, 0b1, 0b0, 0b0);
    case '+': return patternRow(row, 0b0, 0b1, 0b1, 0b1, 0b0);
    case '/': return patternRow(row, 0b001, 0b001, 0b010, 0b100, 0b100);
    case '_': return patternRow(row, 0b000, 0b000, 0b000, 0b000, 0b111);
    default: return 0;
  }
}
