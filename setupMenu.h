#ifndef SETUP_MENU_H
#define SETUP_MENU_H

enum SetupMenuScreen {
  MENU_ROOT = 0,
  MENU_WIFI = 1,
  MENU_TIME = 2,
  MENU_MANUAL_TIME = 3,
  MENU_TIME_ZONE = 4,
  MENU_MESSAGE = 5,
  MENU_WIFI_CLOCK = 6
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
    return 4;
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
    if(setupMenuCursor == 0) return wifiClockEnabled ? "WIFI CLOCK - ON" : "WIFI CLOCK - OFF";
    if(setupMenuCursor == 1) return use24HourClock ? "24 HOUR ON" : "12 HOUR ON";
    if(setupMenuCursor == 2){
      snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%s %+d", timeZones[timeZoneIndex].code, timeZones[timeZoneIndex].offsetHours);
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

void startWifiClockSelection(){
  wifiClockSelection = wifiClockEnabled;
  setupMenuScreen = MENU_WIFI_CLOCK;
  pixelMenu.reset();
}

void toggleWifiClockSelection(){
  wifiClockSelection = !wifiClockSelection;
  pixelMenu.reset();
}

void saveWifiClockSelection(){
  wifiClockEnabled = wifiClockSelection;
  saveCurrentState();
  if(wifiClockEnabled && WiFi.status() == WL_CONNECTED){
    WiFi.hostByName(NTPServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
  }
  setupMenuScreen = MENU_TIME;
  setupMenuCursor = 0;
  pixelMenu.reset();
}

void showWifiClockSelection(unsigned long currentMillis){
  pixelMenu.showText(wifiClockSelection ? "ON" : "OFF", currentMillis, wifiClockSelection ? GREEN : RED);
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
  snprintf(menuTextBuffer, sizeof(menuTextBuffer), "%s %+d", timeZones[timeZoneIndex].code, timeZones[timeZoneIndex].offsetHours);
  pixelMenu.showText(String(menuTextBuffer), currentMillis, GREEN);
}

void setRtcTimeFromUnix(uint32_t unixTime){
  long secondsOfDay = (long)(unixTime % 86400UL) + ((long)timeZones[timeZoneIndex].offsetHours * 3600L);
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
    startWifiClockSelection();
  }else if(setupMenuCursor == 1){
    use24HourClock = !use24HourClock;
    pixelClock.setUse24Hour(use24HourClock);
    saveCurrentState();
    showMenuMessage(use24HourClock ? "24 HOUR ON" : "12 HOUR ON", MENU_TIME, currentMillis);
  }else if(setupMenuCursor == 2){
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
  }else if(setupMenuScreen == MENU_WIFI_CLOCK){
    saveWifiClockSelection();
  }else if(setupMenuScreen == MENU_TIME_ZONE){
    setupMenuScreen = MENU_TIME;
    setupMenuCursor = 2;
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
  }else if(setupMenuScreen == MENU_MANUAL_TIME || setupMenuScreen == MENU_TIME_ZONE || setupMenuScreen == MENU_WIFI_CLOCK){
    byte previousScreen = setupMenuScreen;
    setupMenuScreen = MENU_TIME;
    setupMenuCursor = previousScreen == MENU_TIME_ZONE ? 2 : 0;
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
    }else if(setupMenuScreen == MENU_WIFI_CLOCK){
      toggleWifiClockSelection();
    }else if(setupMenuScreen == MENU_TIME_ZONE){
      adjustTimeZone(1);
    }else if(setupMenuScreen != MENU_MESSAGE){
      moveMenuCursor(-1);
    }
  }

  if(hardwareDownPressed){
    if(setupMenuScreen == MENU_MANUAL_TIME){
      adjustManualTime(-1);
    }else if(setupMenuScreen == MENU_WIFI_CLOCK){
      toggleWifiClockSelection();
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

  if(setupMenuScreen == MENU_WIFI_CLOCK){
    showWifiClockSelection(currentMillis);
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

#endif
