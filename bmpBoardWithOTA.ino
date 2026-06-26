//Fernando Har - 20190424
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "SdFat.h"
//WifiManager-OTA
//#include <ESP8266WiFi.h>   //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h> //https://github.com/Links2004/arduinoWebSockets
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include "ArduinoOTA.h"    //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

#define SD_CS 15           //for SD card reader

#include <Wire.h>        //I2C device 
#include <RtcDS3231.h>  //https://github.com/Makuna/Rtc
RtcDS3231<TwoWire> Rtc(Wire);
#include "PixelBoardController.h"
#include "PixelBoard.h"
#include "PixelClock.h"
#include "pixelMenu.h"
#include "PixelArt.h"
#include "gameSnake.h"
#include "gameTetris.h" //Original game from: https://github.com/scout119/RGB123/tree/master/Tetris
#include "gameOfLife.h"
#include "gameArkanoid.h"


#define OTA_UPDATE 0
#define PIXEL_ART_TRAVERSE 1
#define PIXEL_ART_SINGLE 2
#define SOLID_FILL 3
#define CLOCK 4
#define GAME_SNAKE 5
#define GAME_TETRIS 6 
#define GAME_OF_LIFE 7
#define GAME_ARKANOID 8
#define SETUP_MENU 9
#define TOTAL_MODES 8 

using namespace sdfat;
//PIXEL FRAME 
byte    currentMode = PIXEL_ART_TRAVERSE; //0 - Update Sketch, 1 - Traverse Folders; 2 - Single Folder; 3 - Fill Color; 4 - Clock
byte    previousMode = 255;    //to resume previous mode after OTA update

//SD FILE SYSTEM
SdFat     sd; // File system object.
SdFile    file; // Use for file creation in folders.
bool      sdReady = false;

//LED STRIP 
byte brightness;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(BOARDSIZE, 2, NEO_GRB + NEO_KHZ800);

//WiFiManager
WiFiManager wifiManager;
bool    reconnectWifiFlag;
bool    needRestart = false;

//Web Server
ESP8266WebServer  server(80);//Web server object. Will be listening in port 80 (default for HTTP)
//Web Socket Server
WebSocketsServer webSocket = WebSocketsServer(81);

//Remote Control
PixelBoardController pixelBoardController = PixelBoardController(&webSocket);

//Applications
PixelBoard pixelBoard = PixelBoard(&strip); //Mood light
GameSnake gameSnake = GameSnake(&strip, 3, &pixelBoardController);
GameTetris gameTetris = GameTetris(&strip, &pixelBoardController);
GameOfLife gameOfLife = GameOfLife(&strip, &pixelBoardController);
GameArkanoid gameArkanoid = GameArkanoid(&strip, &pixelBoardController);

PixelArt pixelArt = PixelArt(&strip, &sd, &pixelBoardController);
PixelClock pixelClock = PixelClock(&strip, &Rtc);
PixelMenu pixelMenu = PixelMenu(&strip);

WiFiUDP UDP;

IPAddress timeServerIP;
const char* NTPServerName = "CORPQEHDC02.corp.ha.org.hk";
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

#define EEPROM_CLOCK_FORMAT_ADDRESS 7
#define EEPROM_TIME_ZONE_ADDRESS 8

// ESP8266 projects with NeoPixel + SD/SPI + I2C are pin constrained.
// Set these to GPIO numbers that are free on your board before flashing.
// Use buttons wired to GND; the firmware enables INPUT_PULLUP for each pin.
#ifndef HW_BUTTON_RESET_PIN
#define HW_BUTTON_RESET_PIN -1
#endif
#ifndef HW_BUTTON_SELECT_PIN
#define HW_BUTTON_SELECT_PIN -1
#endif
#ifndef HW_BUTTON_UP_PIN
#define HW_BUTTON_UP_PIN -1
#endif
#ifndef HW_BUTTON_DOWN_PIN
#define HW_BUTTON_DOWN_PIN -1
#endif

#define HW_BUTTON_DEBOUNCE_MS 35
#define HW_BUTTON_REPEAT_DELAY_MS 500
#define HW_BUTTON_REPEAT_MS 180
#define HW_RESET_HOLD_MS 3000

struct TimeZoneSetting {
  const char* code;
  int8_t offsetHours;
};

const TimeZoneSetting TIME_ZONES[] = {
  {"UTC", 0},
  {"PST", -8},
  {"MST", -7},
  {"CST", -6},
  {"EST", -5},
  {"GMT", 0},
  {"CET", 1},
  {"HKT", 8}
};
const byte TIME_ZONE_COUNT = sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]);

bool use24HourClock = true;
byte timeZoneIndex = 0;

enum HardwareButtonId {
  HW_BTN_RESET = 0,
  HW_BTN_SELECT = 1,
  HW_BTN_UP = 2,
  HW_BTN_DOWN = 3,
  HW_BTN_COUNT = 4
};

struct HardwareButtonState {
  int8_t pin;
  bool lastRawPressed;
  bool stablePressed;
  unsigned long lastChange;
  unsigned long pressedAt;
  unsigned long lastRepeat;
  bool longReported;
};

HardwareButtonState hardwareButtons[HW_BTN_COUNT] = {
  {HW_BUTTON_RESET_PIN, false, false, 0, 0, 0, false},
  {HW_BUTTON_SELECT_PIN, false, false, 0, 0, 0, false},
  {HW_BUTTON_UP_PIN, false, false, 0, 0, 0, false},
  {HW_BUTTON_DOWN_PIN, false, false, 0, 0, 0, false}
};

bool hardwareResetShort = false;
bool hardwareResetLong = false;
bool hardwareSelectShort = false;
bool hardwareUpPressed = false;
bool hardwareDownPressed = false;

enum SetupMenuScreen {
  MENU_ROOT = 0,
  MENU_WIFI = 1,
  MENU_TIME = 2,
  MENU_MANUAL_TIME = 3,
  MENU_TIME_ZONE = 4,
  MENU_MESSAGE = 5
};

byte modeBeforeMenu = CLOCK;
byte setupMenuScreen = MENU_ROOT;
byte setupMenuCursor = 0;
byte setupMenuMessageReturnScreen = MENU_ROOT;
byte manualHour = 0;
byte manualMinute = 0;
bool manualEditingHour = true;
String setupMenuMessage = "";
unsigned long setupMenuMessageUntil = 0;
unsigned long pendingRestartAt = 0;
char menuTextBuffer[64];

void setCurrentMode(byte mode);
void setCurrentMode(byte mode, bool persistState);
uint32_t getTime();
void sendNTPpacket(IPAddress& address);

//[Section] - Read and write to EEPROM
byte readEEPROM(uint address){
  return EEPROM.read(address);
}

void saveCurrentState(){
  EEPROM.write(0, currentMode); //Current Mode
  EEPROM.write(1, brightness);//Brightness
  EEPROM.write(2, pixelBoard.ledFill_R);
  EEPROM.write(3, pixelBoard.ledFill_G);
  EEPROM.write(4, pixelBoard.ledFill_B);
  EEPROM.write(5, pixelArt.displaySpeed);
  EEPROM.write(6, pixelArt.currentFolderPointer);
  EEPROM.write(EEPROM_CLOCK_FORMAT_ADDRESS, use24HourClock ? 1 : 0);
  EEPROM.write(EEPROM_TIME_ZONE_ADDRESS, timeZoneIndex);

  EEPROM.commit();
}

void restorePreviousState(){
  byte mode =  readEEPROM(0);
  brightness = readEEPROM(1);
  pixelBoard.ledFill_R = readEEPROM(2);
  pixelBoard.ledFill_G = readEEPROM(3);
  pixelBoard.ledFill_B = readEEPROM(4);
  //Check if Brightness setting is correct
  if(brightness < 1 || brightness > 7){
    brightness = 7;
  }
  
  //Checking if current Mode is correct
  if(mode < OTA_UPDATE || mode > TOTAL_MODES){
    mode = (sdReady) ? PIXEL_ART_TRAVERSE : CLOCK; //
  }
  
  pixelArt.displaySpeed = readEEPROM(5);
  pixelArt.currentFolderPointer = readEEPROM(6);
  byte storedClockFormat = readEEPROM(EEPROM_CLOCK_FORMAT_ADDRESS);
  use24HourClock = storedClockFormat == 0 ? false : true;
  pixelClock.setUse24Hour(use24HourClock);
  timeZoneIndex = readEEPROM(EEPROM_TIME_ZONE_ADDRESS);
  if(timeZoneIndex >= TIME_ZONE_COUNT){
    timeZoneIndex = 0;
  }
  setCurrentMode(mode);
}

void setNextMode(){
	int mode = currentMode + 1;
	if(mode > TOTAL_MODES){
		mode = 1;
	}
  webSocket.broadcastTXT("setNextMode");
	setCurrentMode(mode);
}

void setPreviousMode(){
	int mode = currentMode - 1;
	if(mode < 1){
		mode = TOTAL_MODES;
	}
  webSocket.broadcastTXT("setPreviousMode");
	setCurrentMode(mode);
}

void setCurrentMode(byte mode){
  setCurrentMode(mode, true);
}

void setCurrentMode(byte mode, bool persistState){
	currentMode = mode;
	if(mode == PIXEL_ART_TRAVERSE){	
		pixelArt.startTraverseFolders();
	}else if (mode == PIXEL_ART_SINGLE){
		pixelArt.stopTraverseFolders();
	}else if (mode == GAME_SNAKE){
		gameSnake.reset();
	}else if (mode == GAME_TETRIS){
		gameTetris.reset(); //TODO
	}else if (mode == GAME_OF_LIFE){
		gameOfLife.reset();
	}else if (mode == GAME_ARKANOID){
    gameArkanoid.reset();
	}
  if(persistState && mode != SETUP_MENU){
	  saveCurrentState();
  }
}
//[Section] Handle function for WebSocket ( webSocketEvent() )/ WebServer
void handleTraverse(){
  setCurrentMode(PIXEL_ART_TRAVERSE);
}

void handleSingleFolder(){
  setCurrentMode(PIXEL_ART_SINGLE);
}

void handleShowClock(){
  setCurrentMode(CLOCK);
}

void handleBrighter(){
  setBrightness(++brightness);      
}

void handleDimmer(){
  setBrightness(--brightness);             
}

void handleChangeColor(byte R, byte G, byte B){
  currentMode = SOLID_FILL;
  pixelBoard.fill(R, G, B, true);
  saveCurrentState(); 
}
void handleFaster(){
  pixelArt.faster();
  saveCurrentState(); 
}

void handleSlower(){
	  pixelArt.slower();
	  saveCurrentState(); 
}

void handleGameSnakeMode(){
  setCurrentMode(GAME_SNAKE);
}


//[Section] Setup function for various modules
void setupNeoPixelBoard(){
  Serial.println("[Begin] Setup NeoPixel Board");
  Serial.printf("Board width:%d , height: %d", BOARDWIDTH, BOARDHEIGHT);
  //SETUP NEOPIXEL
  strip.begin();
  strip.show();
  setBrightness(brightness);
  Serial.println("[Complete] Setup NeoPixel Board");
//  Serial.println("[Begin] NeoPixel Board pixel test");
  //pixel test
  pixelBoard.fill(255, 0, 0, false);
  delay(100);
  pixelBoard.fill(0, 255, 0, false);
  delay(100);
  pixelBoard.fill(0, 0, 255, false);
  delay(100);
  pixelBoard.fill(0, 0, 0, false);
  delay(100);
  
  //Serial.println("[Complete] NeoPixel Board pixel test");
  Serial.println("[Complete] Setup NeoPixel Board");
}

bool setupSDCard(){
	return sd.begin(SD_CS, SPI_FULL_SPEED);
}

//[Section] RTC (Real time clock) module
void setRtcDateTime(uint16_t year, uint8_t month, uint8_t dayofMonth, uint8_t hour, uint8_t minute, uint8_t second){
  RtcDateTime dateTime = RtcDateTime(year, month, dayofMonth, hour, minute, second);
  setRtcDateTime(dateTime);
}
void setRtcDateTime(const RtcDateTime& dateTime){
  //RtcDateTime
  Rtc.SetDateTime(dateTime);
}
//--------RTC SETUP ------------
void setupRTC() {
  Serial.println("[Begin] RTC setup ");  
  Rtc.Begin();
  
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    
    if (!Rtc.IsDateTimeValid()){
        // Common Cuases:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing
        Serial.println("Set RTC Time to Compiled Time");
        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue
    setRtcDateTime(compiled);
        //Rtc.SetDateTime(compiled);
  }
  
    if (!Rtc.GetIsRunning()){
        Rtc.SetIsRunning(true);
  }
  
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled){
        //Serial.println("RTC is older than compile time!  (Updating DateTime)");
        //Rtc.SetDateTime(compiled);
    setRtcDateTime(compiled);
  }
  
    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    Serial.println("[Complete] RTC setup ");
}
void readRTC(){
  if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
		return;
	}
  
    RtcDateTime now = Rtc.GetDateTime();
    //printDateTime(now);
    Serial.println();
  
    RtcTemperature temp = Rtc.GetTemperature();
    //temp.Print(Serial);
    // you may also get the temperature as a float and print it
    Serial.print(temp.AsFloatDegC());
    Serial.println("C");
  
}

//File Upload -- https://github.com/G6EJD/ESP32-8266-File-Upload/blob/master/ESP_File_Download_Upload.ino
void File_Upload(){
 String webpage  = "<html><body>";
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  webpage += F("</body></html>");
  server.send(200, "text/html",webpage);
}

sdfat::File uploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& upload = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(upload.status == UPLOAD_FILE_START)
  {     
    String filename = upload.filename;
    if(!filename.startsWith("/")){
    filename = "/"+filename;
  }
   
  sdfat::File delfile;
  
  if(delfile = sd.open(filename, FILE_WRITE)){
    delfile.remove();
    delfile.close();
  }
  
    // Remove a previous version, otherwise data is appended the file again
    uploadFile = sd.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  { 
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } 
  else if (upload.status == UPLOAD_FILE_END)
  {
  if(uploadFile)          // If the file was successfully created
    {                                    
      uploadFile.close();   // Close the file again
    String webpage = "<html><body>";
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); 
    webpage += upload.filename+"</h2>";
      //webpage += F("<h2>File Size: "); webpage += upload.totalSize + "</h2><br>"; 
    webpage += F("</body></html>");
      server.send(200,"text/html",webpage);
    } 
    else
    {
      String webpage = "<html><body><h1>ERROR</h1>File cannot be created</body></html>";
      server.send(200,"text/html",webpage);
    }
  }
}

//[Section] Wifi Management
void connectWiFi(){
  //if(needRestart){
//    Serial.println("********************Reset ESP");
//    delay(2000);
//    needRestart = false;
//    ESP.restart();
//  }
  if(reconnectWifiFlag){
	reconnectWifiFlag = false;
    Serial.println("[Begin] wifi Manager Auto Connect");
	char deviceName[30] = {0}; 
	sprintf(deviceName, "pixelboard_%06X", ESP.getChipId());
	Serial.printf("Device name: [%s]\n", deviceName);
    wifiManager.autoConnect(deviceName);
	
    // start MDNS
    if (MDNS.begin(deviceName)) {
      Serial.println("MDNS responder started.");
    }
   
    Serial.println("WiFi Connected");
    Serial.println(WiFi.localIP());
    Serial.println("[Complete] wifi Manager Auto Connect");

  }
}


void setBrightness(byte level){
  if(level > 7){
    brightness = level = 7;
    }else if (level < 1){
    brightness = level = 1;
  }
  brightness = level;
  saveCurrentState();
  strip.setBrightness(1 << level);
  strip.show();  
}

bool readHardwareButtonRaw(byte index){
  if(hardwareButtons[index].pin < 0){
    return false;
  }
  return digitalRead(hardwareButtons[index].pin) == LOW;
}

void setupHardwareButtons(){
  for(byte i = 0; i < HW_BTN_COUNT; ++i){
    if(hardwareButtons[i].pin >= 0){
      pinMode(hardwareButtons[i].pin, INPUT_PULLUP);
      hardwareButtons[i].lastRawPressed = readHardwareButtonRaw(i);
      hardwareButtons[i].stablePressed = hardwareButtons[i].lastRawPressed;
      hardwareButtons[i].lastChange = millis();
    }
  }
}

void clearHardwareButtonEvents(){
  hardwareResetShort = false;
  hardwareResetLong = false;
  hardwareSelectShort = false;
  hardwareUpPressed = false;
  hardwareDownPressed = false;
}

void updateHardwareButton(byte index, unsigned long currentMillis){
  HardwareButtonState* button = &hardwareButtons[index];
  bool rawPressed = readHardwareButtonRaw(index);

  if(rawPressed != button->lastRawPressed){
    button->lastRawPressed = rawPressed;
    button->lastChange = currentMillis;
  }

  if(currentMillis - button->lastChange < HW_BUTTON_DEBOUNCE_MS){
    return;
  }

  if(rawPressed != button->stablePressed){
    button->stablePressed = rawPressed;
    if(rawPressed){
      button->pressedAt = currentMillis;
      button->lastRepeat = currentMillis;
      button->longReported = false;
    }else if(!button->longReported){
      if(index == HW_BTN_RESET){
        hardwareResetShort = true;
      }else if(index == HW_BTN_SELECT){
        hardwareSelectShort = true;
      }else if(index == HW_BTN_UP){
        hardwareUpPressed = true;
      }else if(index == HW_BTN_DOWN){
        hardwareDownPressed = true;
      }
    }
  }

  if(!button->stablePressed){
    return;
  }

  if(index == HW_BTN_RESET && !button->longReported && currentMillis - button->pressedAt >= HW_RESET_HOLD_MS){
    button->longReported = true;
    hardwareResetLong = true;
  }else if((index == HW_BTN_UP || index == HW_BTN_DOWN) &&
      currentMillis - button->pressedAt >= HW_BUTTON_REPEAT_DELAY_MS &&
      currentMillis - button->lastRepeat >= HW_BUTTON_REPEAT_MS){
    button->lastRepeat = currentMillis;
    if(index == HW_BTN_UP){
      hardwareUpPressed = true;
    }else{
      hardwareDownPressed = true;
    }
  }
}

void readHardwareButtons(unsigned long currentMillis){
  clearHardwareButtonEvents();
  for(byte i = 0; i < HW_BTN_COUNT; ++i){
    updateHardwareButton(i, currentMillis);
  }
}

void showMenuMessage(const String& message, byte returnScreen, unsigned long currentMillis){
  setupMenuMessage = message;
  setupMenuMessageReturnScreen = returnScreen;
  setupMenuMessageUntil = currentMillis + 1800;
  setupMenuScreen = MENU_MESSAGE;
  pixelMenu.reset();
}

void enterSetupMenu(){
  modeBeforeMenu = currentMode;
  if(modeBeforeMenu == SETUP_MENU){
    modeBeforeMenu = CLOCK;
  }
  currentMode = SETUP_MENU;
  setupMenuScreen = MENU_ROOT;
  setupMenuCursor = 0;
  pendingRestartAt = 0;
  pixelMenu.reset();
  pixelBoardController.clearStickyBtns();
}

void exitSetupMenu(){
  byte mode = modeBeforeMenu;
  if(mode < PIXEL_ART_TRAVERSE || mode > TOTAL_MODES){
    mode = CLOCK;
  }
  setCurrentMode(mode, false);
  pixelMenu.reset();
}

byte getMenuItemCount(){
  if(setupMenuScreen == MENU_WIFI){
    return 4;
  }
  if(setupMenuScreen == MENU_TIME){
    return 5;
  }
  return 3;
}

String getCurrentMenuLabel(){
  if(setupMenuScreen == MENU_ROOT){
    if(setupMenuCursor == 0) return "WIFI";
    if(setupMenuCursor == 1) return "TIME";
    return "EXIT";
  }

  if(setupMenuScreen == MENU_WIFI){
    if(setupMenuCursor == 0) return "SHOW IP";
    if(setupMenuCursor == 1) return "SHOW SSID";
    if(setupMenuCursor == 2) return "CONNECT SSID";
    return "BACK";
  }

  if(setupMenuScreen == MENU_TIME){
    if(setupMenuCursor == 0) return "MANUAL TIME";
    if(setupMenuCursor == 1) return use24HourClock ? "24 HOUR ON" : "12 HOUR ON";
    if(setupMenuCursor == 2) return "USE RTC";
    if(setupMenuCursor == 3){
      snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%s %+d", TIME_ZONES[timeZoneIndex].code, TIME_ZONES[timeZoneIndex].offsetHours);
      return String(menuTextBuffer);
    }
    return "BACK";
  }

  return "";
}

void moveMenuCursor(int8_t direction){
  byte itemCount = getMenuItemCount();
  if(direction > 0){
    setupMenuCursor++;
    if(setupMenuCursor >= itemCount){
      setupMenuCursor = 0;
    }
  }else{
    if(setupMenuCursor == 0){
      setupMenuCursor = itemCount - 1;
    }else{
      setupMenuCursor--;
    }
  }
  pixelMenu.reset();
}

void startManualTime(){
  RtcDateTime now = Rtc.GetDateTime();
  manualHour = now.Hour();
  manualMinute = now.Minute();
  manualEditingHour = true;
  setupMenuScreen = MENU_MANUAL_TIME;
  pixelMenu.reset();
}

void saveManualTime(){
  RtcDateTime now = Rtc.GetDateTime();
  setRtcDateTime(now.Year(), now.Month(), now.Day(), manualHour, manualMinute, 0);
}

void adjustManualTime(int8_t direction){
  if(manualEditingHour){
    manualHour = (manualHour + 24 + direction) % 24;
  }else{
    manualMinute = (manualMinute + 60 + direction) % 60;
  }
  pixelMenu.reset();
}

void selectManualTime(unsigned long currentMillis){
  if(manualEditingHour){
    manualEditingHour = false;
  }else{
    saveManualTime();
    manualEditingHour = true;
    showMenuMessage("TIME SAVED", MENU_MANUAL_TIME, currentMillis);
  }
  pixelMenu.reset();
}

void showManualTime(unsigned long currentMillis){
  bool blinkOff = ((currentMillis / 400) % 2) == 0;
  if(manualEditingHour && blinkOff){
    snprintf(menuTextBuffer, sizeof(menuTextBuffer), "__%02d", manualMinute);
  }else if(!manualEditingHour && blinkOff){
    snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%02d__", manualHour);
  }else{
    snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%02d%02d", manualHour, manualMinute);
  }
  pixelMenu.showText(String(menuTextBuffer), currentMillis, ORANGE);
}

void adjustTimeZone(int8_t direction){
  if(direction > 0){
    timeZoneIndex++;
    if(timeZoneIndex >= TIME_ZONE_COUNT){
      timeZoneIndex = 0;
    }
  }else{
    if(timeZoneIndex == 0){
      timeZoneIndex = TIME_ZONE_COUNT - 1;
    }else{
      timeZoneIndex--;
    }
  }
  saveCurrentState();
  pixelMenu.reset();
}

void showTimeZone(unsigned long currentMillis){
  snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%s %+d", TIME_ZONES[timeZoneIndex].code, TIME_ZONES[timeZoneIndex].offsetHours);
  pixelMenu.showText(String(menuTextBuffer), currentMillis, GREEN);
}

void setRtcTimeFromUnix(uint32_t unixTime){
  long secondsOfDay = (long)(unixTime % 86400UL) + ((long)TIME_ZONES[timeZoneIndex].offsetHours * 3600L);
  while(secondsOfDay < 0){
    secondsOfDay += 86400L;
  }
  while(secondsOfDay >= 86400L){
    secondsOfDay -= 86400L;
  }

  RtcDateTime now = Rtc.GetDateTime();
  setRtcDateTime(
    now.Year(),
    now.Month(),
    now.Day(),
    secondsOfDay / 3600,
    (secondsOfDay / 60) % 60,
    secondsOfDay % 60
  );
}

bool syncRtcWithNtp(unsigned long currentMillis){
  if(WiFi.status() != WL_CONNECTED){
    return false;
  }

  WiFi.hostByName(NTPServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  unsigned long startWait = currentMillis;
  while(millis() - startWait < 2000){
    ArduinoOTA.handle();
    webSocket.loop();
    server.handleClient();
    uint32_t unixTime = getTime();
    if(unixTime){
      setRtcTimeFromUnix(unixTime);
      return true;
    }
    delay(10);
  }
  return false;
}

void selectRootMenu(){
  if(setupMenuCursor == 0){
    setupMenuScreen = MENU_WIFI;
    setupMenuCursor = 0;
  }else if(setupMenuCursor == 1){
    setupMenuScreen = MENU_TIME;
    setupMenuCursor = 0;
  }else{
    exitSetupMenu();
  }
  pixelMenu.reset();
}

void selectWifiMenu(unsigned long currentMillis){
  if(setupMenuCursor == 0){
    showMenuMessage(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "NO WIFI", MENU_WIFI, currentMillis);
  }else if(setupMenuCursor == 1){
    String ssid = WiFi.SSID();
    showMenuMessage((WiFi.status() == WL_CONNECTED && ssid.length() > 0) ? ssid : "NO SSID", MENU_WIFI, currentMillis);
  }else if(setupMenuCursor == 2){
    wifiManager.resetSettings();
    pendingRestartAt = currentMillis + 2200;
    showMenuMessage("WIFI SETUP RESTART", MENU_WIFI, currentMillis);
  }else{
    setupMenuScreen = MENU_ROOT;
    setupMenuCursor = 0;
    pixelMenu.reset();
  }
}

void selectTimeMenu(unsigned long currentMillis){
  if(setupMenuCursor == 0){
    startManualTime();
  }else if(setupMenuCursor == 1){
    use24HourClock = !use24HourClock;
    pixelClock.setUse24Hour(use24HourClock);
    saveCurrentState();
    showMenuMessage(use24HourClock ? "24 HOUR ON" : "12 HOUR ON", MENU_TIME, currentMillis);
  }else if(setupMenuCursor == 2){
    if(WiFi.status() != WL_CONNECTED){
      showMenuMessage("SETUP WIFI FIRST", MENU_TIME, currentMillis);
    }else if(syncRtcWithNtp(currentMillis)){
      snprintf(menuTextBuffer, sizeof(menuTextBuffer), "RTC %s %+d", TIME_ZONES[timeZoneIndex].code, TIME_ZONES[timeZoneIndex].offsetHours);
      showMenuMessage(String(menuTextBuffer), MENU_TIME, millis());
    }else{
      showMenuMessage("NTP FAILED", MENU_TIME, millis());
    }
  }else if(setupMenuCursor == 3){
    setupMenuScreen = MENU_TIME_ZONE;
    pixelMenu.reset();
  }else{
    setupMenuScreen = MENU_ROOT;
    setupMenuCursor = 0;
    pixelMenu.reset();
  }
}

void selectSetupMenu(unsigned long currentMillis){
  if(setupMenuScreen == MENU_ROOT){
    selectRootMenu();
  }else if(setupMenuScreen == MENU_WIFI){
    selectWifiMenu(currentMillis);
  }else if(setupMenuScreen == MENU_TIME){
    selectTimeMenu(currentMillis);
  }else if(setupMenuScreen == MENU_MANUAL_TIME){
    selectManualTime(currentMillis);
  }else if(setupMenuScreen == MENU_TIME_ZONE){
    setupMenuScreen = MENU_TIME;
    setupMenuCursor = 3;
    saveCurrentState();
    showMenuMessage("TZ SAVED", MENU_TIME, currentMillis);
  }
}

void handleSetupMenuBack(){
  if(setupMenuScreen == MENU_ROOT){
    exitSetupMenu();
  }else if(setupMenuScreen == MENU_WIFI || setupMenuScreen == MENU_TIME){
    setupMenuScreen = MENU_ROOT;
    setupMenuCursor = 0;
  }else if(setupMenuScreen == MENU_MANUAL_TIME || setupMenuScreen == MENU_TIME_ZONE){
    byte previousScreen = setupMenuScreen;
    setupMenuScreen = MENU_TIME;
    setupMenuCursor = previousScreen == MENU_TIME_ZONE ? 3 : 0;
  }else if(setupMenuScreen == MENU_MESSAGE){
    setupMenuScreen = setupMenuMessageReturnScreen;
  }
  pixelMenu.reset();
}

void handleSetupMenuButtons(unsigned long currentMillis){
  if(hardwareSelectShort){
    selectSetupMenu(currentMillis);
  }

  if(hardwareUpPressed){
    if(setupMenuScreen == MENU_MANUAL_TIME){
      adjustManualTime(1);
    }else if(setupMenuScreen == MENU_TIME_ZONE){
      adjustTimeZone(1);
    }else if(setupMenuScreen != MENU_MESSAGE){
      moveMenuCursor(-1);
    }
  }

  if(hardwareDownPressed){
    if(setupMenuScreen == MENU_MANUAL_TIME){
      adjustManualTime(-1);
    }else if(setupMenuScreen == MENU_TIME_ZONE){
      adjustTimeZone(-1);
    }else if(setupMenuScreen != MENU_MESSAGE){
      moveMenuCursor(1);
    }
  }
}

void updateSetupMenu(unsigned long currentMillis){
  if(pendingRestartAt > 0 && currentMillis >= pendingRestartAt){
    ESP.restart();
  }

  if(setupMenuScreen == MENU_MESSAGE){
    pixelMenu.showText(setupMenuMessage, currentMillis, YELLOW);
    if(currentMillis >= setupMenuMessageUntil){
      setupMenuScreen = setupMenuMessageReturnScreen;
      pixelMenu.reset();
    }
    return;
  }

  if(setupMenuScreen == MENU_MANUAL_TIME){
    showManualTime(currentMillis);
    return;
  }

  if(setupMenuScreen == MENU_TIME_ZONE){
    showTimeZone(currentMillis);
    return;
  }

  pixelMenu.showText(getCurrentMenuLabel(), currentMillis, CYAN);
}

void triggerHardwareReset(unsigned long currentMillis){
  pixelMenu.showText("RESET", currentMillis, RED);
  saveCurrentState();
  delay(500);
  ESP.restart();
}

unsigned long loopTimerTemp = 0;
unsigned long ntpTimer = 0;

const char JSON_DEBUGGER_FORMAT[] = "{\"msgType\":\"debugInfo\",\"uptime\":\"%s\",\"sdReady\":%d, \"currentFolderPointer\":%d, \"rootFolderCount\":%d, \"currentMode\":%d, \"displaySpeed\":%d, \"brightness\":%d, \"freeHeap\":%d}";
const char UPTIMECHAR_FORMAT[] = "%02dd:%02dh:%02dm:%02ds";
char uptimeChar[15];
char jsonBuffer[200];
uint32_t timeUNIX = 0;
unsigned long lastNTPResponse = 0;
unsigned long prevActualTime = 0;
void loop() {
  //connectWiFi();
  ArduinoOTA.handle();
  webSocket.loop();
  server.handleClient();
  
  loopTimerTemp = millis();

  readHardwareButtons(loopTimerTemp);
  if(hardwareResetLong){
    triggerHardwareReset(loopTimerTemp);
    return;
  }

  if(hardwareResetShort){
    if(currentMode == SETUP_MENU){
      handleSetupMenuBack();
    }else{
      enterSetupMenu();
    }
  }

  if(currentMode == SETUP_MENU){
    handleSetupMenuButtons(loopTimerTemp);
    updateSetupMenu(loopTimerTemp);
  }else{
    if(currentMode != OTA_UPDATE){
      if(hardwareUpPressed){
        setPreviousMode();
      }else if(hardwareDownPressed){
        setNextMode();
      }
    }
 
  if(pixelBoardController.getBtnStatus(BTNS) == 1 &&
    pixelBoardController.getBtnStatus(BTNA) == 1 ){
	
  
  
  //if((loopTimerTemp - debuggTimer) > 1000){
  //Debugging information via websocket
  //Uptime (how long the program run since last restart)
  int sec = loopTimerTemp / 1000;
  int min = sec / 60;
  int hr = min / 60;
  int day = hr / 24;
  
  //Print the uptime (String) to uptimeChar
  snprintf(uptimeChar, 15, UPTIMECHAR_FORMAT, day, hr % 24, min % 60, sec % 60);
  
  //Print Debugger information (JSON) to jsonBuffer
  snprintf(jsonBuffer, 200, JSON_DEBUGGER_FORMAT, uptimeChar, sdReady, pixelArt.currentFolderPointer, pixelArt.rootFolderCount, currentMode, pixelArt.displaySpeed, brightness, ESP.getFreeHeap());
  webSocket.broadcastTXT(jsonBuffer);
  //debuggTimer = loopTimerTemp;
  return;
}


  if(pixelBoardController.getStickyBtnStatus(BTNS) == 1){
      setNextMode();//Change Mode
      pixelBoardController.clearStickyBtns();
  }else if ( currentMode == GAME_SNAKE){
	  gameSnake.update(loopTimerTemp); //Show Game
  }else if (currentMode == GAME_TETRIS){
	  gameTetris.update(loopTimerTemp);
  }else if (currentMode == GAME_OF_LIFE){
	 gameOfLife.update(loopTimerTemp);
  }else if (currentMode == GAME_ARKANOID){
   gameArkanoid.update(loopTimerTemp);
  }else if((currentMode == PIXEL_ART_TRAVERSE || currentMode == PIXEL_ART_SINGLE ) && sdReady){
  	pixelArt.update(loopTimerTemp); //Show Pixel art
  }else if (currentMode == SOLID_FILL){
    pixelBoard.update(loopTimerTemp); //Fill Board
  }else if (currentMode == CLOCK){
    pixelClock.update(loopTimerTemp); //Show Clock
  }
  }
  if(loopTimerTemp - ntpTimer > 300000){
	ntpTimer = loopTimerTemp;
	 sendNTPpacket(timeServerIP);               // Send an NTP request
  }
  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    Serial.println("NTP response:\t");
    Serial.println(timeUNIX);
	lastNTPResponse = loopTimerTemp;
	uint32_t actualTime = timeUNIX + (loopTimerTemp - lastNTPResponse)/1000;
	if (actualTime != prevActualTime && timeUNIX != 0) { // If a second has passed since last print
		prevActualTime = actualTime;
		Serial.printf("\rUTC time:\t%d:%d:%d   \n", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime));
	}  
  }
}

bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) {
    path += "pixBoardController.htm";
  }

  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
    } else if (path.endsWith(".htm")) {
    dataType = "text/html";
    } else if (path.endsWith(".css")) {
    dataType = "text/css";
    } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
    } else if (path.endsWith(".png")) {
    dataType = "image/png";
    } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
    } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
    } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
    } else if (path.endsWith(".xml")) {
    dataType = "text/xml";
    } else if (path.endsWith(".pdf")) {
    dataType = "application/pdf";
    } else if (path.endsWith(".zip")) {
    dataType = "application/zip";
  }
  sdfat::File myfile;
  sd.vwd()->rewind();
  if(sd.exists(path.c_str()) ){
    myfile = sd.open(path.c_str(), O_READ);
    server.streamFile( myfile , dataType);
    myfile.close();
    }else{
    return false;
  }
  file.close();
  return true;
}
void handleNotFound() {
  
  if (sdReady && loadFromSdCard(server.uri())) {
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//[Section] Web Socket Event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght){
  
  switch (type) {
    case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    webSocket.disconnect(num);
    webSocket.broadcastTXT("client disconnect");
    break;
    
    case WStype_CONNECTED:
    {
       IPAddress ip = webSocket.remoteIP(num);
      //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    const char FORMAT[] = "[%u] Connected from %d.%d.%d.%d url: %s\n";
    char buffer[100];
    snprintf(buffer, 100, FORMAT,  num, ip[0], ip[1], ip[2], ip[3], payload);
      webSocket.sendTXT(num, buffer);
      webSocket.sendTXT(num, "{\"status\":\"Connected\"}");
    }
    break;
    
    case WStype_TEXT:{
      //Serial.printf("[%u] got Text: %s\n", num, payload);
      //webSocket.broadcastTXT(payload);
      String text = String((char *) &payload[0]);
      if(text.equals("traverse")){
        handleTraverse();	
      }else if(text.equals("singleFolder")){
        handleSingleFolder();
      }else if (text.equals("showClock")){
        handleShowClock();
      }else if (text.equals("bright")){
        handleBrighter();
      }else if (text.equals("dim")){
        handleDimmer();
      }else if(text.equals("faster")){
        handleFaster();
      }else if(text.equals("slower")){
        handleSlower();
      }else if(text.startsWith("changeColor")){
        String colorCode = text.substring(12); //text  = changeColor#0,0,255
        char buf[sizeof(colorCode)];
        colorCode.toCharArray(buf, sizeof(buf));
        char *p = buf;
        char *str;
        handleChangeColor(atoi(strtok(p , ",")), atoi(strtok(NULL , ",")), atoi(strtok(NULL , ",")));
      }else if(text.equals("startGameSnake")){
        handleGameSnakeMode();
      }else if(text.startsWith("buttonStatus:")){
		 uint8_t i = 13;
		  //uint8_t key = payload[i++]; 
      //webSocket.broadcastTXT((const char*)&key); // 1 
      //webSocket.broadcastTXT((const char*)&payload[13]); //1000000

     uint8_t key = payload[i++]; 
     pixelBoardController.setBtnStatus(UP, atoi((const char*)&key));
     key = payload[i++];
     pixelBoardController.setBtnStatus(DOWN, atoi((const char*)&key));
     key = payload[i++];
	 pixelBoardController.setBtnStatus(LEFT, atoi((const char*)&key));
     key = payload[i++];
     pixelBoardController.setBtnStatus(RIGHT, atoi((const char*)&key));
	 key = payload[i++];
     pixelBoardController.setBtnStatus(BTNS, atoi((const char*)&key));
    key = payload[i++];
     pixelBoardController.setBtnStatus(BTNA, atoi((const char*)&key));
      key = payload[i++];
     pixelBoardController.setBtnStatus(BTNB, atoi((const char*)&key)); 
  
      }
    }
    break;
    
    case WStype_BIN:
    Serial.printf("[%u] got binary length: %u\n", num, lenght);
    hexdump(payload, lenght);
    break;
    
    default:
    Serial.println("webSocketEvent else");
  }
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

//[Section] Main Control UI HTML

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  pixelBoardController.begin();
  setupHardwareButtons();
  
  sdReady = setupSDCard();
  if(sdReady){
    Serial.println("[SD Card] is Ready");
	pixelArt.init();
  }else{
    Serial.println("[SD CARD] ERROR");
    
  }
  EEPROM.begin(512);
  restorePreviousState(); 

  setupNeoPixelBoard();
  reconnectWifiFlag = true;
  needRestart = false;
  
  setupRTC();
  Serial.println("Display ICON");
  pixelArt.displayIcon("wifi.bmp");
  
  wifiManager.setConfigPortalTimeout(60);
  connectWiFi();

  if(WiFi.status() == WL_CONNECTED){
	//Sync RTC with NTP;
	
	Serial.println("Starting UDP");
	UDP.begin(123);                          // Start listening for UDP messages on port 123
	Serial.print("Local port:\t");
	Serial.println(UDP.localPort());
  
	WiFi.hostByName(NTPServerName, timeServerIP);
	 Serial.print("Time server IP:\t");
	Serial.println(timeServerIP);
  
	Serial.println("\r\nSending NTP request ...");
	sendNTPpacket(timeServerIP);  
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);

  server.on("/snakeGame", handleGameSnakeMode);
  server.on("/gameSnake", handleGameSnakeMode);
  server.on("/upload",File_Upload);
  server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);
  server.onNotFound(handleNotFound);
  server.begin(); //Web Server
  }
  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  
  ArduinoOTA.onStart([]() {   
    currentMode = OTA_UPDATE;
    pixelBoard.fill(249, 105, 255, false); //Set Screen to Purple when uploading 
    
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH){
      type = "sketch";
      }else{
      type = "filesystem";  // U_SPIFFS
    }
    
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    restorePreviousState();    
    pixelBoard.fill(255, 255, 44, false); //Set Screen to yellow when uploading completed
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    pixelBoard.fill(255, 0, 0, false); //Set Screen to Red when Error uploading 
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("Setup completed");
}
