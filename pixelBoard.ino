//Fernando Har - 20190424
#include <SPI.h>
#include <EEPROM.h>
#include <string.h>
#include <Adafruit_NeoPixel.h>
#include "SdFat.h"
#include "sdios.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h> //https://github.com/Links2004/arduinoWebSockets
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define NEOPIXEL_PIN 21
#define RTC_SDA_PIN 8
#define RTC_SCL_PIN 9

#define SD_CS 10
#define SD_SCK 12
#define SD_MISO 13
#define SD_MOSI 11

#if HAS_SDIO_CLASS
  #define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
  #define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI)
#else
  #define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI)
#endif

#include <Wire.h>        //I2C device 
#include <RtcDS3231.h>  //https://github.com/Makuna/Rtc
RtcDS3231<TwoWire> Rtc(Wire);
#include "pixelBoardController.h"
#include "pixelBoard.h"
#include "pixelClock.h"
#include "pixelMenu.h"
#include "pixelMood.h"
#include "pixelArt.h"
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
#define MOOD_LIGHT 9
#define BLE_DISPLAY 10
#define SETUP_MENU 11
#define TOTAL_MODES 9

//PIXEL FRAME 
byte    currentMode = PIXEL_ART_TRAVERSE; //0 - Update Sketch, 1 - Traverse Folders; 2 - Single Folder; 3 - Fill Color; 4 - Clock
byte    previousMode = 255;    //to resume previous mode after OTA update

//SD FILE SYSTEM
SdFat32   sd; // File system object.
File32    file; // Use for file creation in folders.
bool      sdReady = false;

//LED STRIP 
byte brightness;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(BOARDSIZE, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

//WiFiManager
WiFiManager wifiManager;
bool    reconnectWifiFlag;
bool    needRestart = false;

//Web Server
WebServer  server(80);//Web server object. Will be listening in port 80 (default for HTTP)
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
PixelMood pixelMood = PixelMood(&strip);

PixelArt pixelArt = PixelArt(&strip, &sd, &pixelBoardController);
PixelClock pixelClock = PixelClock(&strip, &Rtc);
PixelMenu pixelMenu = PixelMenu(&strip);

WiFiUDP UDP;

IPAddress timeServerIP;
#define NTP_SERVER_NAME_MAX_LENGTH 63
const char* DEFAULT_NTP_SERVER_NAME = "pool.ntp.org";
const char* NTP_SERVER_CONFIG_FILE = "/ntpserver.txt";
const char* NTP_INTERVAL_CONFIG_FILE = "/ntpinterval.txt";
char NTPServerName[NTP_SERVER_NAME_MAX_LENGTH + 1] = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
#define NTP_SYNC_DEFAULT_SECONDS 300UL
#define NTP_SYNC_MIN_SECONDS 30UL
#define NTP_SYNC_MAX_SECONDS 86400UL
#define NTP_RESPONSE_TIMEOUT_MS 5000UL
unsigned long ntpSyncIntervalMs = NTP_SYNC_DEFAULT_SECONDS * 1000UL;
unsigned long ntpCurrentIntervalMs = NTP_SYNC_DEFAULT_SECONDS * 1000UL;
unsigned long ntpTimer = 0;
unsigned long ntpRequestSentAt = 0;
bool ntpRequestPending = false;

#define BLE_SERVICE_UUID "7d8f0001-6f8a-4a5a-9d6b-40f520dc0001"
#define BLE_COMMAND_UUID "7d8f0002-6f8a-4a5a-9d6b-40f520dc0001"
#define BLE_STATUS_UUID  "7d8f0003-6f8a-4a5a-9d6b-40f520dc0001"
#define BLE_WIFI_CONNECT_TIMEOUT_MS 20000UL
BLECharacteristic* bleStatusCharacteristic = NULL;
bool bleClientConnected = false;
bool bleWifiPending = false;
bool bleWifiConnecting = false;
unsigned long bleWifiStartedAt = 0;
String bleWifiSsid = "";
String bleWifiPassword = "";
String bleDisplayText = "";
bool bleDisplayTextActive = false;

#define EEPROM_CLOCK_FORMAT_ADDRESS 7
#define EEPROM_TIME_ZONE_ADDRESS 8
#define EEPROM_WIFI_CLOCK_ADDRESS 9
#define EEPROM_WIFI_SETUP_MESSAGE_ADDRESS 10
#define EEPROM_MOOD_PRESET_ADDRESS 11
#define WIFI_SETUP_MESSAGE_MS 6500

// ESP8266 projects with NeoPixel + SD/SPI + I2C are pin constrained.
// Set these to GPIO numbers that are free on your board before flashing.
// Use buttons wired to GND; the firmware enables INPUT_PULLUP for each pin.
#ifndef HW_BUTTON_RESET_PIN
#define HW_BUTTON_RESET_PIN 4
#endif
#ifndef HW_BUTTON_SELECT_PIN
#define HW_BUTTON_SELECT_PIN 5
#endif
#ifndef HW_BUTTON_UP_PIN
#define HW_BUTTON_UP_PIN 6
#endif
#ifndef HW_BUTTON_DOWN_PIN
#define HW_BUTTON_DOWN_PIN 7
#endif

#define HW_BUTTON_DEBOUNCE_MS 35
#define HW_BUTTON_REPEAT_DELAY_MS 500
#define HW_BUTTON_REPEAT_MS 180
#define HW_RESET_HOLD_MS 3000
#define TIME_ZONE_LABEL_MAX_LENGTH 7
#define TIME_ZONE_COUNTRY_CODE_MAX_LENGTH 2
#define TIME_ZONE_CONFIG_LINE_MAX_LENGTH 32
#define TIME_ZONE_GLOBAL_CONFIG_FILE "/timezone_global.conf"
#define TIME_ZONE_COUNTRY_CONFIG_FILE "/timezone_country.conf"

struct TimeZoneSetting {
  char code[TIME_ZONE_LABEL_MAX_LENGTH + 1];
  int8_t offsetHours;
};

const TimeZoneSetting DEFAULT_TIME_ZONES[] = {
  {"BAK", -12},
  {"PPG", -11},
  {"HNL", -10},
  {"ANC", -9},
  {"LA", -8},
  {"DEN", -7},
  {"CHI", -6},
  {"NYC", -5},
  {"SCL", -4},
  {"RIO", -3},
  {"FEN", -2},
  {"PDL", -1},
  {"LON", 0},
  {"PAR", 1},
  {"CAI", 2},
  {"MOW", 3},
  {"DXB", 4},
  {"KHI", 5},
  {"DAC", 6},
  {"BKK", 7},
  {"SHA", 8},
  {"TOK", 9},
  {"SYD", 10},
  {"NOU", 11},
  {"AKL", 12},
  {"TBU", 13},
  {"CXI", 14}
};
const byte TIME_ZONE_COUNT = sizeof(DEFAULT_TIME_ZONES) / sizeof(DEFAULT_TIME_ZONES[0]);
TimeZoneSetting timeZones[TIME_ZONE_COUNT];
char timeZoneCountryCode[TIME_ZONE_COUNTRY_CODE_MAX_LENGTH + 1] = "CA";

bool use24HourClock = true;
bool wifiClockEnabled = true;
bool wifiClockSelection = true;
bool wifiSetupPortalRequested = false;
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

void setCurrentMode(byte mode);
void setCurrentMode(byte mode, bool persistState);
uint32_t getTime();
void sendNTPpacket(IPAddress& address);
void setRtcTimeFromUnix(uint32_t unixTime);
void scheduleNtpSyncNow();
void setupBLE();
void processBleTasks(unsigned long currentMillis);
void updateBleDisplay(unsigned long currentMillis);
void handleBleCommand(String command);
void enterSetupMenu();
void handleSetupMenuBack();
void handleSetupMenuButtons(unsigned long currentMillis);
void updateSetupMenu(unsigned long currentMillis);
void triggerHardwareReset(unsigned long currentMillis);

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
  EEPROM.write(EEPROM_WIFI_CLOCK_ADDRESS, wifiClockEnabled ? 1 : 0);
  EEPROM.write(EEPROM_MOOD_PRESET_ADDRESS, pixelMood.getPreset());

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
  byte storedWifiClock = readEEPROM(EEPROM_WIFI_CLOCK_ADDRESS);
  wifiClockEnabled = storedWifiClock == 0 ? false : true;
  wifiSetupPortalRequested = readEEPROM(EEPROM_WIFI_SETUP_MESSAGE_ADDRESS) == 1;
  byte storedMoodPreset = readEEPROM(EEPROM_MOOD_PRESET_ADDRESS);
  if(storedMoodPreset < MOOD_PRESET_COUNT){
    pixelMood.setPreset(storedMoodPreset);
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
	}else if (mode == MOOD_LIGHT){
    pixelMood.reset();
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
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
	return sd.begin(SD_CONFIG);
}

bool isNtpServerNameSafeChar(char c){
  return (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    (c >= '0' && c <= '9') ||
    c == '.' || c == '-' || c == '_' || c == ':';
}

bool loadNtpServerFromSD(){
  strncpy(NTPServerName, DEFAULT_NTP_SERVER_NAME, NTP_SERVER_NAME_MAX_LENGTH);
  NTPServerName[NTP_SERVER_NAME_MAX_LENGTH] = '\0';

  if(!sdReady || !sd.exists(NTP_SERVER_CONFIG_FILE)){
    Serial.printf("NTP server: %s (default)\n", NTPServerName);
    return false;
  }

  File32 configFile = sd.open(NTP_SERVER_CONFIG_FILE, O_RDONLY);
  if(!configFile){
    Serial.printf("NTP server: %s (default, config open failed)\n", NTPServerName);
    return false;
  }

  char line[NTP_SERVER_NAME_MAX_LENGTH + 1] = {0};
  byte lineLength = 0;
  bool hasValue = false;
  bool skipLine = false;
  int input;

  while((input = configFile.read()) >= 0){
    char c = (char)input;
    if(c == '\r'){
      continue;
    }
    if(c == '\n'){
      line[lineLength] = '\0';
      if(lineLength > 0){
        hasValue = true;
        break;
      }
      lineLength = 0;
      line[0] = '\0';
      skipLine = false;
      continue;
    }
    if(skipLine){
      continue;
    }
    if(c == '#'){
      skipLine = true;
      continue;
    }
    if(lineLength == 0 && (c == ' ' || c == '\t')){
      continue;
    }
    if(lineLength < NTP_SERVER_NAME_MAX_LENGTH && isNtpServerNameSafeChar(c)){
      line[lineLength++] = c;
    }
  }

  if(!hasValue && lineLength > 0){
    line[lineLength] = '\0';
    hasValue = true;
  }

  configFile.close();

  if(!hasValue){
    Serial.printf("NTP server: %s (default, config empty)\n", NTPServerName);
    return false;
  }

  strncpy(NTPServerName, line, NTP_SERVER_NAME_MAX_LENGTH);
  NTPServerName[NTP_SERVER_NAME_MAX_LENGTH] = '\0';
  Serial.printf("NTP server: %s (from SD %s)\n", NTPServerName, NTP_SERVER_CONFIG_FILE);
  return true;
}

bool loadNtpIntervalFromSD(){
  ntpSyncIntervalMs = NTP_SYNC_DEFAULT_SECONDS * 1000UL;
  ntpCurrentIntervalMs = ntpSyncIntervalMs;

  if(!sdReady || !sd.exists(NTP_INTERVAL_CONFIG_FILE)){
    Serial.printf("NTP interval: %lu seconds (default)\n", ntpSyncIntervalMs / 1000UL);
    return false;
  }

  File32 configFile = sd.open(NTP_INTERVAL_CONFIG_FILE, O_RDONLY);
  if(!configFile){
    Serial.printf("NTP interval: %lu seconds (default, config open failed)\n", ntpSyncIntervalMs / 1000UL);
    return false;
  }

  char value[11] = {0};
  byte valueLength = 0;
  bool hasValue = false;
  bool skipLine = false;
  int input;

  while((input = configFile.read()) >= 0){
    char c = (char)input;
    if(c == '\r'){
      continue;
    }
    if(c == '\n'){
      if(valueLength > 0){
        hasValue = true;
        break;
      }
      skipLine = false;
      continue;
    }
    if(skipLine){
      continue;
    }
    if(c == '#'){
      skipLine = true;
      continue;
    }
    if(c >= '0' && c <= '9' && valueLength < sizeof(value) - 1){
      value[valueLength++] = c;
    }else if(valueLength > 0){
      hasValue = true;
      break;
    }
  }

  configFile.close();

  if(!hasValue && valueLength == 0){
    Serial.printf("NTP interval: %lu seconds (default, config empty)\n", ntpSyncIntervalMs / 1000UL);
    return false;
  }

  unsigned long seconds = strtoul(value, NULL, 10);
  if(seconds < NTP_SYNC_MIN_SECONDS){
    seconds = NTP_SYNC_MIN_SECONDS;
  }else if(seconds > NTP_SYNC_MAX_SECONDS){
    seconds = NTP_SYNC_MAX_SECONDS;
  }

  ntpSyncIntervalMs = seconds * 1000UL;
  ntpCurrentIntervalMs = ntpSyncIntervalMs;
  Serial.printf("NTP interval: %lu seconds (from SD %s)\n", seconds, NTP_INTERVAL_CONFIG_FILE);
  return true;
}

void copyDefaultTimeZones(){
  for(byte i = 0; i < TIME_ZONE_COUNT; ++i){
    strncpy(timeZones[i].code, DEFAULT_TIME_ZONES[i].code, TIME_ZONE_LABEL_MAX_LENGTH);
    timeZones[i].code[TIME_ZONE_LABEL_MAX_LENGTH] = '\0';
    timeZones[i].offsetHours = DEFAULT_TIME_ZONES[i].offsetHours;
  }
}

bool isTimeZoneLabelChar(char c){
  return (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    (c >= '0' && c <= '9') ||
    c == '_' || c == '-';
}

bool isOffsetToken(const char* token){
  byte index = 0;
  if(token[0] == '+' || token[0] == '-'){
    index = 1;
  }
  if(token[index] == '\0'){
    return false;
  }
  while(token[index] != '\0'){
    if(token[index] < '0' || token[index] > '9'){
      return false;
    }
    index++;
  }
  return true;
}

void uppercaseTimeZoneLabel(char* label){
  for(byte i = 0; label[i] != '\0'; ++i){
    if(label[i] >= 'a' && label[i] <= 'z'){
      label[i] -= 32;
    }
  }
}

int findTimeZoneIndexByOffset(int8_t offsetHours){
  for(byte i = 0; i < TIME_ZONE_COUNT; ++i){
    if(timeZones[i].offsetHours == offsetHours){
      return i;
    }
  }
  return -1;
}

bool applyTimeZoneLabel(const char* label, int8_t offsetHours){
  int index = findTimeZoneIndexByOffset(offsetHours);
  if(index < 0 || label[0] == '\0'){
    return false;
  }

  strncpy(timeZones[index].code, label, TIME_ZONE_LABEL_MAX_LENGTH);
  timeZones[index].code[TIME_ZONE_LABEL_MAX_LENGTH] = '\0';
  uppercaseTimeZoneLabel(timeZones[index].code);
  return true;
}

bool parseTimeZoneConfigLine(char* line, char* label, int8_t* offsetHours){
  char tokens[2][TIME_ZONE_CONFIG_LINE_MAX_LENGTH + 1] = {{0}, {0}};
  byte tokenCount = 0;
  byte tokenLength = 0;
  bool inToken = false;

  for(byte i = 0; line[i] != '\0' && line[i] != '#'; ++i){
    char c = line[i];
    bool tokenChar = isTimeZoneLabelChar(c) || c == '+';
    if(tokenChar){
      if(tokenCount < 2 && tokenLength < TIME_ZONE_CONFIG_LINE_MAX_LENGTH){
        tokens[tokenCount][tokenLength++] = c;
      }
      inToken = true;
    }else if(inToken){
      if(tokenCount < 2){
        tokens[tokenCount][tokenLength] = '\0';
        tokenCount++;
      }
      tokenLength = 0;
      inToken = false;
    }
  }

  if(inToken && tokenCount < 2){
    tokens[tokenCount][tokenLength] = '\0';
    tokenCount++;
  }

  if(tokenCount < 2){
    return false;
  }

  if(isOffsetToken(tokens[0])){
    *offsetHours = atoi(tokens[0]);
    strncpy(label, tokens[1], TIME_ZONE_LABEL_MAX_LENGTH);
  }else if(isOffsetToken(tokens[1])){
    *offsetHours = atoi(tokens[1]);
    strncpy(label, tokens[0], TIME_ZONE_LABEL_MAX_LENGTH);
  }else{
    return false;
  }

  label[TIME_ZONE_LABEL_MAX_LENGTH] = '\0';
  return *offsetHours >= -12 && *offsetHours <= 14;
}

bool loadTimeZoneFile(const char* path){
  if(!sdReady || !sd.exists(path)){
    return false;
  }

  File32 configFile = sd.open(path, O_RDONLY);
  if(!configFile){
    return false;
  }

  char line[TIME_ZONE_CONFIG_LINE_MAX_LENGTH + 1] = {0};
  byte lineLength = 0;
  byte appliedCount = 0;
  int input;

  while((input = configFile.read()) >= 0){
    char c = (char)input;
    if(c == '\r'){
      continue;
    }
    if(c == '\n'){
      line[lineLength] = '\0';
      char label[TIME_ZONE_LABEL_MAX_LENGTH + 1] = {0};
      int8_t offsetHours = 0;
      if(parseTimeZoneConfigLine(line, label, &offsetHours) && applyTimeZoneLabel(label, offsetHours)){
        appliedCount++;
      }
      lineLength = 0;
      line[0] = '\0';
      continue;
    }
    if(lineLength < TIME_ZONE_CONFIG_LINE_MAX_LENGTH){
      line[lineLength++] = c;
    }
  }

  if(lineLength > 0){
    line[lineLength] = '\0';
    char label[TIME_ZONE_LABEL_MAX_LENGTH + 1] = {0};
    int8_t offsetHours = 0;
    if(parseTimeZoneConfigLine(line, label, &offsetHours) && applyTimeZoneLabel(label, offsetHours)){
      appliedCount++;
    }
  }

  configFile.close();
  Serial.printf("Timezone config %s applied %d entries\n", path, appliedCount);
  return appliedCount > 0;
}

bool loadTimeZoneCountryFromSD(){
  strncpy(timeZoneCountryCode, "CA", TIME_ZONE_COUNTRY_CODE_MAX_LENGTH);
  timeZoneCountryCode[TIME_ZONE_COUNTRY_CODE_MAX_LENGTH] = '\0';

  if(!sdReady || !sd.exists(TIME_ZONE_COUNTRY_CONFIG_FILE)){
    return false;
  }

  File32 configFile = sd.open(TIME_ZONE_COUNTRY_CONFIG_FILE, O_RDONLY);
  if(!configFile){
    return false;
  }

  byte length = 0;
  int input;
  while((input = configFile.read()) >= 0){
    char c = (char)input;
    if(c == '#'){
      break;
    }
    if(c == '\r' || c == '\n'){
      if(length > 0){
        break;
      }
      continue;
    }
    if(c == ' ' || c == '\t'){
      if(length == 0){
        continue;
      }
      break;
    }
    if(length < TIME_ZONE_COUNTRY_CODE_MAX_LENGTH && isTimeZoneLabelChar(c)){
      timeZoneCountryCode[length++] = c;
    }
  }

  configFile.close();
  if(length == 0){
    strncpy(timeZoneCountryCode, "CA", TIME_ZONE_COUNTRY_CODE_MAX_LENGTH);
    timeZoneCountryCode[TIME_ZONE_COUNTRY_CODE_MAX_LENGTH] = '\0';
    return false;
  }
  timeZoneCountryCode[length] = '\0';
  uppercaseTimeZoneLabel(timeZoneCountryCode);
  return length > 0;
}

void loadTimeZonesFromSD(){
  copyDefaultTimeZones();

  if(!sdReady){
    Serial.println("Timezone config: defaults only (SD unavailable)");
    return;
  }

  loadTimeZoneFile(TIME_ZONE_GLOBAL_CONFIG_FILE);
  loadTimeZoneCountryFromSD();

  char countryFile[24] = "/";
  strncat(countryFile, timeZoneCountryCode, TIME_ZONE_COUNTRY_CODE_MAX_LENGTH);
  strncat(countryFile, "_timezone.conf", sizeof(countryFile) - strlen(countryFile) - 1);

  if(loadTimeZoneFile(countryFile)){
    Serial.printf("Timezone country: %s\n", timeZoneCountryCode);
  }else{
    Serial.printf("Timezone country file not loaded: %s\n", countryFile);
  }
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
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
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
void handleFileUploadForm(){
 String webpage  = "<html><body>";
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  webpage += F("</body></html>");
  server.send(200, "text/html",webpage);
}

File32 uploadFile;
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& upload = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(upload.status == UPLOAD_FILE_START)
  {     
    String filename = upload.filename;
    if(!filename.startsWith("/")){
    filename = "/"+filename;
  }
   
  if(sd.exists(filename.c_str())){
    sd.remove(filename.c_str());
  }
  
    // Remove a previous version, otherwise data is appended the file again
    uploadFile = sd.open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC);  // Open the file for writing on the SD card.
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
void formatDeviceName(char* deviceName, size_t deviceNameSize){
  uint32_t chipId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
  snprintf(deviceName, deviceNameSize, "pixelboard_%06X", chipId);
}

String getWifiSetupInstruction(){
  char deviceName[30] = {0};
  formatDeviceName(deviceName, sizeof(deviceName));
  return String("WIFI CREDENTIAL CLEARED CONNECT TO ") + deviceName + " TO SETUP WIFI";
}

void showScrollingMessage(const String& message, unsigned long durationMs, uint32_t color){
  unsigned long startedAt = millis();
  pixelMenu.reset();
  while(millis() - startedAt < durationMs){
    pixelMenu.showText(message, millis(), color);
    delay(40);
    yield();
  }
}

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
	formatDeviceName(deviceName, sizeof(deviceName));
	Serial.printf("Device name: [%s]\n", deviceName);
    if(wifiSetupPortalRequested){
      String instruction = getWifiSetupInstruction();
      Serial.println(instruction);
      showScrollingMessage(instruction, WIFI_SETUP_MESSAGE_MS, CYAN);
    }
    wifiManager.autoConnect(deviceName);
	
    // start MDNS
    if (MDNS.begin(deviceName)) {
      Serial.println("MDNS responder started.");
    }
   
    Serial.println("WiFi Connected");
    Serial.println(WiFi.localIP());
    if(WiFi.status() == WL_CONNECTED && wifiSetupPortalRequested){
      wifiSetupPortalRequested = false;
      EEPROM.write(EEPROM_WIFI_SETUP_MESSAGE_ADDRESS, 0);
      EEPROM.commit();
    }
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

byte hexValue(char c){
  if(c >= '0' && c <= '9') return c - '0';
  if(c >= 'A' && c <= 'F') return c - 'A' + 10;
  if(c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

byte parseHexByte(const String& value, int offset){
  return (hexValue(value.charAt(offset)) << 4) | hexValue(value.charAt(offset + 1));
}

uint32_t parseHexColor(const String& value, int offset){
  byte red = parseHexByte(value, offset);
  byte green = parseHexByte(value, offset + 2);
  byte blue = parseHexByte(value, offset + 4);
  return strip.Color(red, green, blue);
}

void notifyBleStatus(const String& status){
  Serial.println(status);
  if(bleStatusCharacteristic != NULL && bleClientConnected){
    bleStatusCharacteristic->setValue(status.c_str());
    bleStatusCharacteristic->notify();
  }
}

void setBleDisplayMode(){
  currentMode = BLE_DISPLAY;
  bleDisplayTextActive = false;
}

void handleBleWifiCommand(const String& payload){
  int separator = payload.indexOf('|');
  if(separator < 0){
    separator = payload.indexOf(',');
  }
  if(separator <= 0){
    notifyBleStatus("ERR WIFI USE WIFI:ssid|password");
    return;
  }

  bleWifiSsid = payload.substring(0, separator);
  bleWifiPassword = payload.substring(separator + 1);
  bleWifiPending = true;
  bleWifiConnecting = false;
  notifyBleStatus("WIFI CONNECT QUEUED");
}

void handleBlePixelCommand(const String& payload){
  int firstComma = payload.indexOf(',');
  int secondComma = payload.indexOf(',', firstComma + 1);
  if(firstComma < 0 || secondComma < 0 || payload.length() < secondComma + 7){
    notifyBleStatus("ERR PIX USE PIX:x,y,RRGGBB");
    return;
  }

  int x = payload.substring(0, firstComma).toInt();
  int y = payload.substring(firstComma + 1, secondComma).toInt();
  if(x < 0 || x >= BOARDWIDTH || y < 0 || y >= BOARDHEIGHT){
    notifyBleStatus("ERR PIX RANGE");
    return;
  }

  setBleDisplayMode();
  strip.setPixelColor(pixelMenu.getPixelIndex((byte)x, (byte)y), parseHexColor(payload, secondComma + 1));
  strip.show();
  notifyBleStatus("OK PIX");
}

void handleBleRowCommand(const String& payload){
  int separator = payload.indexOf(':');
  if(separator < 0){
    notifyBleStatus("ERR ROW USE ROW:y:RGBHEX");
    return;
  }

  int y = payload.substring(0, separator).toInt();
  String rowData = payload.substring(separator + 1);
  if(y < 0 || y >= BOARDHEIGHT || rowData.length() < BOARDWIDTH * 6){
    notifyBleStatus("ERR ROW RANGE");
    return;
  }

  setBleDisplayMode();
  for(byte x = 0; x < BOARDWIDTH; ++x){
    strip.setPixelColor(pixelMenu.getPixelIndex(x, (byte)y), parseHexColor(rowData, x * 6));
  }
  strip.show();
  notifyBleStatus("OK ROW");
}

void handleBleFrameCommand(const String& payload){
  if(payload.length() < BOARDSIZE * 6){
    notifyBleStatus("ERR FRAME NEED 1536 HEX");
    return;
  }

  setBleDisplayMode();
  for(byte y = 0; y < BOARDHEIGHT; ++y){
    for(byte x = 0; x < BOARDWIDTH; ++x){
      int offset = ((y * BOARDWIDTH) + x) * 6;
      strip.setPixelColor(pixelMenu.getPixelIndex(x, y), parseHexColor(payload, offset));
    }
  }
  strip.show();
  notifyBleStatus("OK FRAME");
}

void handleBleCommand(String command){
  command.trim();
  if(command.length() == 0){
    return;
  }

  if(command.startsWith("WIFI:")){
    handleBleWifiCommand(command.substring(5));
  }else if(command.startsWith("TEXT:")){
    bleDisplayText = command.substring(5);
    bleDisplayTextActive = true;
    currentMode = BLE_DISPLAY;
    pixelMenu.reset();
    notifyBleStatus("OK TEXT");
  }else if(command.equals("CLEAR")){
    setBleDisplayMode();
    for(int i = 0; i < BOARDSIZE; ++i){
      strip.setPixelColor(i, BLACK);
    }
    strip.show();
    notifyBleStatus("OK CLEAR");
  }else if(command.startsWith("PIX:")){
    handleBlePixelCommand(command.substring(4));
  }else if(command.startsWith("ROW:")){
    handleBleRowCommand(command.substring(4));
  }else if(command.startsWith("FRAME:")){
    handleBleFrameCommand(command.substring(6));
  }else if(command.startsWith("BRIGHT:")){
    setBrightness((byte)command.substring(7).toInt());
    notifyBleStatus("OK BRIGHT");
  }else{
    notifyBleStatus("ERR UNKNOWN COMMAND");
  }
}

class PixelBoardBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    bleClientConnected = true;
  }

  void onDisconnect(BLEServer* server) {
    bleClientConnected = false;
    BLEDevice::startAdvertising();
  }
};

class PixelBoardBleCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) {
    String command = characteristic->getValue().c_str();
    handleBleCommand(command);
  }
};

void setupBLE(){
  char deviceName[30] = {0};
  formatDeviceName(deviceName, sizeof(deviceName));
  BLEDevice::init(deviceName);

  BLEServer* bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new PixelBoardBleServerCallbacks());
  BLEService* service = bleServer->createService(BLE_SERVICE_UUID);

  BLECharacteristic* commandCharacteristic = service->createCharacteristic(
    BLE_COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setCallbacks(new PixelBoardBleCommandCallbacks());

  bleStatusCharacteristic = service->createCharacteristic(
    BLE_STATUS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  bleStatusCharacteristic->addDescriptor(new BLE2902());
  bleStatusCharacteristic->setValue("PIXELBOARD READY");

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("BLE control service started");
}

void processBleTasks(unsigned long currentMillis){
  if(bleWifiPending && !bleWifiConnecting){
    bleWifiConnecting = true;
    bleWifiStartedAt = currentMillis;
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.begin(bleWifiSsid.c_str(), bleWifiPassword.c_str());
    notifyBleStatus("WIFI CONNECTING");
  }

  if(bleWifiConnecting){
    if(WiFi.status() == WL_CONNECTED){
      bleWifiPending = false;
      bleWifiConnecting = false;
      notifyBleStatus(String("WIFI CONNECTED ") + WiFi.localIP().toString());
      if(wifiClockEnabled){
        scheduleNtpSyncNow();
      }
    }else if(currentMillis - bleWifiStartedAt > BLE_WIFI_CONNECT_TIMEOUT_MS){
      bleWifiPending = false;
      bleWifiConnecting = false;
      notifyBleStatus("WIFI CONNECT FAILED");
    }
  }
}

void updateBleDisplay(unsigned long currentMillis){
  if(bleDisplayTextActive){
    pixelMenu.showText(bleDisplayText, currentMillis, WHITE);
  }
}

#include "setupMenu.h"

unsigned long loopTimerTemp = 0;

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
  processBleTasks(loopTimerTemp);

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
      if(currentMode == MOOD_LIGHT){
        if(hardwareSelectShort){
          setNextMode();
        }else if(hardwareUpPressed){
          pixelMood.previousPreset();
          saveCurrentState();
        }else if(hardwareDownPressed){
          pixelMood.nextPreset();
          saveCurrentState();
        }
      }else{
        if(hardwareUpPressed){
          setPreviousMode();
        }else if(hardwareDownPressed){
          setNextMode();
        }
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
  }else if (currentMode == MOOD_LIGHT){
    pixelMood.update(loopTimerTemp); //Mood light / RGB randomizer
  }else if (currentMode == BLE_DISPLAY){
    updateBleDisplay(loopTimerTemp); //BLE phone display/message board
  }
  }
  handleNtpSchedule(loopTimerTemp);
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
    if(wifiClockEnabled){
      setRtcTimeFromUnix(timeUNIX);
      ntpRequestPending = false;
      resetNtpBackoff();
      ntpTimer = loopTimerTemp;
      Serial.println("RTC synced from WiFi clock");
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
  File32 myfile;
  if(sd.exists(path.c_str()) ){
    myfile = sd.open(path.c_str(), O_RDONLY);
    server.setContentLength(myfile.size());
    server.send(200, dataType, "");
    uint8_t buffer[512];
    int bytesRead;
    WiFiClient client = server.client();
    while((bytesRead = myfile.read(buffer, sizeof(buffer))) > 0){
      client.write(buffer, bytesRead);
      yield();
    }
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

void resetNtpBackoff(){
  ntpCurrentIntervalMs = ntpSyncIntervalMs;
}

void increaseNtpBackoff(){
  unsigned long maxIntervalMs = NTP_SYNC_MAX_SECONDS * 1000UL;
  if(ntpCurrentIntervalMs > maxIntervalMs / 2){
    ntpCurrentIntervalMs = maxIntervalMs;
  }else{
    ntpCurrentIntervalMs *= 2;
  }
  Serial.printf("Next NTP attempt in %lu seconds\n", ntpCurrentIntervalMs / 1000UL);
}

void scheduleNtpSyncNow(){
  ntpRequestPending = false;
  resetNtpBackoff();
  ntpTimer = millis() - ntpCurrentIntervalMs;
}

void startNtpRequest(unsigned long currentMillis){
  if(WiFi.status() != WL_CONNECTED){
    ntpTimer = currentMillis;
    ntpRequestPending = false;
    Serial.println("NTP skipped: WiFi not connected");
    increaseNtpBackoff();
    return;
  }

  WiFi.hostByName(NTPServerName, timeServerIP);
  Serial.print("NTP request to:\t");
  Serial.println(timeServerIP);
  sendNTPpacket(timeServerIP);
  ntpTimer = currentMillis;
  ntpRequestSentAt = currentMillis;
  ntpRequestPending = true;
}

void handleNtpSchedule(unsigned long currentMillis){
  if(!wifiClockEnabled){
    ntpRequestPending = false;
    return;
  }

  if(ntpRequestPending && currentMillis - ntpRequestSentAt > NTP_RESPONSE_TIMEOUT_MS){
    ntpRequestPending = false;
    Serial.println("NTP response timeout");
    increaseNtpBackoff();
  }

  if(!ntpRequestPending && currentMillis - ntpTimer >= ntpCurrentIntervalMs){
    startNtpRequest(currentMillis);
  }
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
  loadNtpServerFromSD();
  loadNtpIntervalFromSD();
  loadTimeZonesFromSD();
  EEPROM.begin(512);
  restorePreviousState(); 

  setupNeoPixelBoard();
  reconnectWifiFlag = true;
  needRestart = false;
  
  setupRTC();
  Serial.println("Display ICON");
  pixelArt.displayIcon("wifi.bmp");
  
  setupBLE();
  wifiManager.setConfigPortalTimeout(60);
  connectWiFi();

  if(WiFi.status() == WL_CONNECTED){
	// Start UDP so WiFi clock can sync RTC from NTP when enabled.
	
	Serial.println("Starting UDP");
	UDP.begin(123);                          // Start listening for UDP messages on port 123
	Serial.print("Local port:\t");
	Serial.println(123);
  
	WiFi.hostByName(NTPServerName, timeServerIP);
	 Serial.print("Time server IP:\t");
	Serial.println(timeServerIP);
  
  if(wifiClockEnabled){
	  Serial.println("\r\nScheduling NTP request ...");
	  scheduleNtpSyncNow();
  }
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);

  server.on("/snakeGame", handleGameSnakeMode);
  server.on("/gameSnake", handleGameSnakeMode);
  server.on("/upload", handleFileUploadForm);
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
