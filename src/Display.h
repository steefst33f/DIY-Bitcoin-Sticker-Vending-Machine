#include <Arduino.h>

#pragma once

#ifndef Display_h
#define Display_h

 // #define M5STACK // comment out if not using M5Stack
  #define TTGO // comment out if not using TTGO
  // #define ILI9431_240x320 // comment out if not using ILI9431 240x320

  #ifdef ILI9431_240x320
    #define TFT_BL 13
  #endif

  #if defined(M5STACK) || defined(TTGO) || defined(ILI9431_240x320)
    #define TFT_DISPLAY
  #endif
  
  #ifdef TFT_DISPLAY
    #include <TFT_eSPI.h>
    #include <QRCode.h>

    #define BLACK TFT_BLACK
    #define WHITE TFT_WHITE
    #define RED TFT_RED
    #define GREEN TFT_GREEN
    #define PURPLE TFT_PURPLE
    #define YELLOW TFT_YELLOW
  #else
    #define BLACK 0x0000
    #define PURPLE 0x780F
    #define GREEN 0x07E0
    #define RED 0xF800
    #define WHITE 0xFFFF
    #define YELLOW 0xFFE0
  #endif

    void initDisplay();
    void displayWifiCredentials(String apSSID, String apPassword, String apIp);
    void displayWifiSetupQrCode(String qrCodeData);
    void displayConnectingToWifi();
    void displayWifiConnected(String ssid, String localIp);
    void displayErrorMessage(String text);
    void displayInfoMessage(String text);
    void displaySuccessMessage(String text);
    void drawMultilineText(String text);

#endif