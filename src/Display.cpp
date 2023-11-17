#include "Display.h"

#if SHOW_MY_DISPLAY_DEBUG_SERIAL
#define MY_DISPLAY_DEBUG_SERIAL Serial
#endif

#ifdef TFT_DISPLAY
  #include <TFT_eSPI.h>
  #include <qrcode.h>
  
  TFT_eSPI tft = TFT_eSPI();
  int qrScreenBrightness = 180;
  uint16_t qrScreenBgColour = tft.color565(qrScreenBrightness, qrScreenBrightness, qrScreenBrightness);  

  QRCode createQrCode(String data);
  void drawQrCode(QRCode qrCode, TFT_eSPI tft, int pixelSize, int xPosition, int yPosition);

#endif

void clearDisplay(uint16_t color){
  #ifdef TFT_DISPLAY
    tft.fillScreen(color);
  #endif
}

void initDisplay() {  
  #ifdef TFT_DISPLAY
    // Setup tft screen
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
  #endif
}

void displayLogo() {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawCentreString("LN Vending Machine", tft.width() / 2, 10, 4);
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawCentreString("v2.0.0 by Steef", tft.width() / 2, 55, 2);
  tft.setTextColor(TFT_ORANGE);
  tft.drawCentreString("Powered by LNBITS", tft.width() / 2, 90, 4);
#endif
}

void displayVendorMode() {
#ifdef TFT_DISPLAY
  String title = "Vendor Mode";
  String text = "Top button: Refill\n"; 
  text += "Bottom button: Dispense\n\n"; 
  text += "Reset to exit\n"; 
  displayScreen(title, text);
#endif
}

void displayWifiCredentials(String apSSID, String apPassword, String apIp) {
#ifdef TFT_DISPLAY
  String title = "WiFi Configuration";
  String text = "SSID: " + apSSID + "\n"; 
  text += "PassW: " + apPassword + "\n"; 
  text += "IP: " + apIp + "\n"; 
  displayScreen(title, text);
#endif
}

void displayWifiSetupQrCode(String qrCodeData) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Configuration", 2, 5, 4); 
  // QrCode AP
  int qrCodePixelSize = 3;
  QRCode qrCodeAp = createQrCode(qrCodeData);
  drawQrCode(qrCodeAp, tft, qrCodePixelSize, 70, 32);
#endif
}

#ifdef TFT_DISPLAY
QRCode createQrCode(String data) {
  #ifdef MY_DISPLAY_DEBUG_SERIAL
  MY_DISPLAY_DEBUG_SERIAL.println(data);
  #endif
  const char *qrDataChar = data.c_str();
  QRCode qrCode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];

  qrcode_initText(&qrCode, qrcodeData, 4, 0, qrDataChar);
  return qrCode;
}
#endif

#ifdef TFT_DISPLAY
void drawQrCode(QRCode qrCode, TFT_eSPI tft, int pixelSize, int xOffset, int yOffSet) {
  #ifdef MY_DISPLAY_DEBUG_SERIAL
  MY_DISPLAY_DEBUG_SERIAL.print("\n\n\n\n");
  #endif

  for (uint8_t y = 0; y < qrCode.size; y++) {
    // Left quiet zone
    #ifdef MY_DISPLAY_DEBUG_SERIAL
    MY_DISPLAY_DEBUG_SERIAL.print("        ");
    #endif
    for (uint8_t x = 0; x < qrCode.size; x++) {
      if (qrcode_getModule(&qrCode, x, y)) {
        tft.fillRect(xOffset + pixelSize * x, yOffSet + pixelSize * y, pixelSize, pixelSize, TFT_BLACK);
        #ifdef MY_DISPLAY_DEBUG_SERIAL
        MY_DISPLAY_DEBUG_SERIAL.print("\u2588"); // Print each module (UTF-8 \u2588 is a solid block)
        #endif
      } else {
        tft.fillRect(xOffset + pixelSize * x, yOffSet + pixelSize * y, pixelSize, pixelSize, TFT_WHITE);
        #ifdef MY_DISPLAY_DEBUG_SERIAL
        MY_DISPLAY_DEBUG_SERIAL.print(" ");
        #endif
      }
    }
    #ifdef MY_DISPLAY_DEBUG_SERIAL
    MY_DISPLAY_DEBUG_SERIAL.print("\n");
    #endif
  }
  #ifdef MY_DISPLAY_DEBUG_SERIAL
  MY_DISPLAY_DEBUG_SERIAL.print("\n\n\n\n");
  #endif
}
#endif

void displayConnectingToWifi() {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Configuration", 2, 5, 4);   tft.setTextPadding(tft.width()); // Remove text padding
  String longText = "Connecting to Wifi...";
  tft.drawCentreString(longText, tft.width() / 2, tft.height() / 2, 2);
#endif
}

void displayPriceToPay(String price) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(BLACK);
  tft.drawString("Scan card to pay..", 2, 10, 2);   
  tft.setTextPadding(tft.width()); // Remove text padding
  tft.setTextColor(TFT_DARKCYAN);
  String bigText = "Price: " + price + " Sats";
  tft.drawCentreString(bigText, tft.width() / 2, (tft.height() / 2), 4);
#endif
}

void displayPayed(String price) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(BLACK);
  tft.drawString("Whoehoe Payed!", 2, 10, 2);   
  tft.setTextPadding(tft.width()); // Remove text padding
  tft.setTextColor(TFT_DARKCYAN);
  String bigText = "You payed " + price + "Sats!";
  tft.drawCentreString(bigText, tft.width() / 2, (tft.height() / 2), 4);
  showConfetti();
#endif
}

void showConfetti() {
#ifdef TFT_DISPLAY
  unsigned long startTime = millis();
  struct Confetti {
    uint8_t x, y, size;
    uint16_t origColor;
  };
  
  uint8_t numberOfConfetti = 60;
  Confetti lastConfetti[numberOfConfetti]; // To keep track of the last frame's confetti
  
  while (millis() - startTime < 4000) {  // Run for approximately 3 seconds
    // Draw new confetti and save their details
    for(int i = 0; i < numberOfConfetti; i++) {
      uint8_t x = random(0, tft.width());
      uint8_t y = random(0, tft.height());
      uint8_t size = random(2, 6);  // Random size between 2 and 5
      uint16_t origColor = tft.readPixel(x, y);  // Store the original color

      uint16_t color = random(0xFFFF);
      for(uint8_t dx = 0; dx < size; dx++) {
        for(uint8_t dy = 0; dy < size; dy++) {
          tft.drawPixel(x + dx, y + dy, color);
        }
      }
      lastConfetti[i] = {x, y, size, origColor};
    }
    
    delay(150); // Refresh rate

    // Restore original colors from last frame's confetti
    for(uint8_t i = 0; i < numberOfConfetti; i++) {
      for(uint8_t dx = 0; dx < lastConfetti[i].size; dx++) {
        for(uint8_t dy = 0; dy < lastConfetti[i].size; dy++) {
          tft.drawPixel(lastConfetti[i].x + dx, lastConfetti[i].y + dy, lastConfetti[i].origColor);
        }
      }
    }
  }
#endif
}

void displayWifiConnected(String ssid, String localIp) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Connected", 2, 5, 4); 
  String wifiInfo = "SSID: " + ssid + "\n";
  wifiInfo += "IP address: " + localIp + "\n";
  setDisplayText(wifiInfo, BLACK, WHITE, 2, 0, 40);
#endif
}

void displayMessage(String text) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextWrap(true, true);
  tft.println(text);
#endif
}

void drawMultilineText(String text, uint8_t textFont/* = 2*/) {
#ifdef TFT_DISPLAY
    tft.setTextFont(textFont);
  // Split the text into lines
    int lineHeight = tft.fontHeight() * 2; // Adjust line height as needed
    int y = (tft.height() - lineHeight * 3) / 2; // Calculate vertical position

    // Display each line of text
    String line;
    int startPos = 0;
    int endPos = 0;

    for (int i = 0; i < 5; i++) { // Display up to 5 lines (adjust as needed)
        endPos = text.indexOf('\n', startPos);
        if (endPos == -1) {
            line = text.substring(startPos);
        } else {
            line = text.substring(startPos, endPos);
            startPos = endPos + 1;
        }

        tft.drawCentreString(line, tft.width() / 2, y, 2);
        y += lineHeight;
    }
#endif
}

void setDisplayText(String text, uint16_t textColor, uint16_t backgroundColor, uint8_t textFont /* = 2*/, int x /* = 0*/, int y /* = 0*/) {
  #ifdef TFT_DISPLAY
    tft.setCursor(x, y);
    tft.setTextFont(textFont);
    tft.setTextColor(textColor);
    tft.setTextWrap(true);
    tft.println(text);
  #endif
  #ifdef MY_DISPLAY_DEBUG_SERIAL
  MY_DISPLAY_DEBUG_SERIAL.println(text);
  #endif
}

void debugDisplayText(String text) {
  #if DEBUG == 1
    #if defined(TTGO)
      clearDisplay(TFT_WHITE); 
      setDisplayText(text, GREEN, BLACK);
    #endif
  #endif
}

void displayScreen(String title, String body) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString(title, 2, 5, 4); 
  tft.setTextColor(BLACK);
  setDisplayText(body, BLACK, WHITE, 2, 5, 40);
#endif
}

void displayErrorScreen(String title, String body) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(RED);
  tft.drawString(title, 2, 5, 4); 
  tft.setTextColor(BLACK);
  setDisplayText(body, BLACK, WHITE, 2, 5, 40);
#endif
}

void displayDebugInfoScreen(String title, String body) {
#ifdef TFT_DISPLAY
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(GREEN);
  tft.drawString(title, 2, 5, 4); 
  tft.setTextColor(BLACK);
  setDisplayText(body, GREEN, BLACK, 2, 5, 40);
#endif
}
