//Fernando Har - 20190424
#include <SPI.h>
#include <EEPROM.h>
#include <SdFat.h>
#include <IniFileLite.h>
#include <Adafruit_NeoPixel.h>

//WifiManager-OTA
#include <ESP8266WiFi.h>   //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h> //https://github.com/Links2004/arduinoWebSockets
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include "ArduinoOTA.h"    //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

#define SD_CS 15           //for SD card reader

#include <Wire.h>        //I2C device 
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
#include "PixelBoardController.h"
#include "PixelBoard.h"
#include "PixelClock.h"
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
#define TOTAL_MODES 8 

//PIXEL FRAME 
byte    currentMode = PIXEL_ART_TRAVERSE; //0 - Update Sketch, 1 - Traverse Folders; 2 - Single Folder; 3 - Fill Color; 4 - Clock
byte    previousMode = 255;    //to resume previous mode after OTA update

//SD FILE SYSTEM
SdFat     sd; // File system object.
SdFile    file; // Use for file creation in folders.
bool      sdReady = false;

//LED STRIP 
byte brightness;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(256, 2, NEO_GRB + NEO_KHZ800);

//WiFiManager
WiFiManager wifiManager;
bool    reconnectWifiFlag;
bool    needRestart = false;

//Web Server
ESP8266WebServer  server(80);//Web server object. Will be listening in port 80 (default for HTTP)
//Web Socket Server
WebSocketsServer webSocket = WebSocketsServer(81);

//PixelBoardController pixelBoardController = PixelBoardController();
PixelBoardController pixelBoardController = PixelBoardController(&webSocket);
PixelBoard pixelBoard = PixelBoard(&strip);

//GAME
GameSnake gameSnake = GameSnake(&strip, 3, &pixelBoardController); //avoid copy;
GameTetris gameTetris = GameTetris(&strip, &pixelBoardController);
GameOfLife gameOfLife = GameOfLife(&strip, &pixelBoardController);
GameArkanoid gameArkanoid = GameArkanoid(&strip, &pixelBoardController);

PixelArt pixelArt = PixelArt(&strip, &sd, &pixelBoardController);
PixelClock pixelClock = PixelClock(&strip, &Rtc);


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
void setCurrentMode(byte mode){
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
	saveCurrentState();
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


void setupNeoPixelBoard(){
  Serial.println("[Begin] Setup NeoPixel Board");
  //SETUP NEOPIXEL
  strip.begin();
  strip.show();
  setBrightness(brightness);
  Serial.println("[Complete] Setup NeoPixel Board");
  Serial.println("[Begin] NeoPixel Board pixel test");
  //pixel test
  pixelBoard.fill(255, 0, 0, false);
  delay(100);
  pixelBoard.fill(0, 255, 0, false);
  delay(100);
  pixelBoard.fill(0, 0, 255, false);
  delay(100);
  pixelBoard.fill(0, 0, 0, false);
  delay(100);
  
  Serial.println("[Complete] NeoPixel Board pixel test");
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

File uploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& upload = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(upload.status == UPLOAD_FILE_START)
  {     
    String filename = upload.filename;
    if(!filename.startsWith("/")){
    filename = "/"+filename;
  }
   
  File delfile;
  
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
    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("pixelboard-0001");
	//wifiManager.startConfigPortal("PixelBoard");
    // start MDNS
    if (MDNS.begin("pixelboard-0001")) {
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

unsigned long loopTimerTemp = 0;
unsigned long debuggTimer = 0;

const char JSON_DEBUGGER_FORMAT[] = "{\"msgType\":\"debugInfo\",\"uptime\":\"%s\",\"sdReady\":%d, \"currentFolderPointer\":%d, \"rootFolderCount\":%d, \"currentMode\":%d, \"displaySpeed\":%d, \"brightness\":%d, \"freeHeap\":%d}";
const char UPTIMECHAR_FORMAT[] = "%02dd:%02dh:%02dm:%02ds";
char uptimeChar[15];
char jsonBuffer[200];

void loop() {
  //connectWiFi();
  ArduinoOTA.handle();
  webSocket.loop();
  server.handleClient();
  
  loopTimerTemp = millis();
 
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
  debuggTimer = loopTimerTemp;
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
  File myfile;
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

//[Section] Main Control UI HTML

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  
  sdReady = setupSDCard();
  
  EEPROM.begin(512);
  if(sdReady){
    //pixelArt = PixelArt(&strip, &sd);
	pixelArt.init();
  }
  restorePreviousState(); 

  setupNeoPixelBoard();
  reconnectWifiFlag = true;
  needRestart = false;
  
  setupRTC();
  wifiManager.setConfigPortalTimeout(60);
  connectWiFi();

  if(WiFi.status() == WL_CONNECTED){

  
  
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
