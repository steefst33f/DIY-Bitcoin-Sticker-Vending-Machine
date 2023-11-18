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
    #include <qrcode.h>

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
    void displayLogo();
    void displayVendorMode();
    void displayWifiCredentials(String apSSID, String apPassword, String apIp);
    void displayWifiSetupQrCode(String qrCodeData);
    void displayConnectingToWifi();
    void displayWifiConnected(String ssid, String localIp);
    void displayPriceToPay(String price);
    void displayPayed(String price);

    void showConfetti();
    void drawMultilineText(String text, uint8_t textFont = 2);
    void setDisplayText(String text, uint16_t textColor, uint16_t backgroundColor, uint8_t textFont = 2, int x = 0, int y = 0);
    void setDisplayInfoText(String text);
    void setDisplaySuccessText(String text);
    void setDisplayErrorText(String text);
    void debugDisplayText(String text);
    void displayScreen(String title, String body);
    void displayErrorScreen(String title, String body);
    void displayDebugInfoScreen(String title, String body);

#endif
