#include "Display.h"

#ifdef TFT_DISPLAY
  #include <TFT_eSPI.h>
  #include <qrcode.h>
  
  TFT_eSPI tft = TFT_eSPI();
  int qrScreenBrightness = 180;
  uint16_t qrScreenBgColour = tft.color565(qrScreenBrightness, qrScreenBrightness, qrScreenBrightness);  
#endif

QRCode createQrCode(String data);
void drawQrCode(QRCode qrCode, TFT_eSPI tft, int pixelSize, int xPosition, int yPosition);

void clearDisplay(uint16_t color){
  #ifdef TFT_DISPLAY
    tft.fillScreen(color);
  #endif
}

void initDisplay() {
  #ifdef TFT_DISPLAY
    // Setup tft screen
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_WHITE); // Clear the screen
    tft.setTextColor(TFT_BLACK);
  #endif
}

void displayWifiCredentials(String apSSID, String apPassword, String apIp) {
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Setup", 2, 5, 4);
  String text = "SSID: " + apSSID + "\n"; 
  text += "PassW: " + apPassword + "\n"; 
  text += "IP: " + apIp + "\n"; 
  tft.setTextColor(TFT_BLACK);
  uint16_t maxWidth = tft.width();// /2;
  uint16_t maxHeight = tft.height(); 
  drawMultilineText(text);
}

void displayWifiSetupQrCode(String qrCodeData) {
  clearDisplay(TFT_WHITE); 
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Setup", 2, 5, 4); 
  // QrCode AP
  int qrCodePixelSize = 3;
  QRCode qrCodeAp = createQrCode(qrCodeData);
  drawQrCode(qrCodeAp, tft, qrCodePixelSize, 70, 35);
}

QRCode createQrCode(String data) {
  Serial.println(data);
  
  const char *qrDataChar = data.c_str();
  QRCode qrCode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];

  qrcode_initText(&qrCode, qrcodeData, 4, 0, qrDataChar);
  return qrCode;
}

void drawQrCode(QRCode qrCode, TFT_eSPI tft, int pixelSize, int xOffset, int yOffSet) {
  Serial.print("\n\n\n\n");

  for (uint8_t y = 0; y < qrCode.size; y++) {
    // Left quiet zone
    Serial.print("        ");
    for (uint8_t x = 0; x < qrCode.size; x++) {
      if (qrcode_getModule(&qrCode, x, y)) {
        tft.fillRect(xOffset + pixelSize * x, yOffSet + pixelSize * y, pixelSize, pixelSize, TFT_BLACK);
        Serial.print("\u2588"); // Print each module (UTF-8 \u2588 is a solid block)
      } else {
        tft.fillRect(xOffset + pixelSize * x, yOffSet + pixelSize * y, pixelSize, pixelSize, TFT_WHITE);
        Serial.print(" ");
      }
    }
    Serial.print("\n");
  }
  Serial.print("\n\n\n\n");
}

void displayConnectingToWifi() {
  clearDisplay(TFT_WHITE); // Clear the screen
  tft.setTextPadding(tft.width()); // Remove text padding
  String longText = "Connecting to Wifi...";
  tft.drawCentreString(longText, tft.width() / 2, tft.height() / 2, 2);
}

void displayWifiConnected(String ssid, String localIp) {
  String wifiInfo = "Connected to WiFi\n";
  wifiInfo += "SSID: " + ssid + "\n";
  wifiInfo += "IP address: " + localIp + "\n";

  tft.fillScreen(TFT_WHITE); // Clear the screen
  drawMultilineText(wifiInfo);
}

void displayErrorMessage(String text) {
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_RED);
  tft.setTextFont(2);
  tft.setTextWrap(true, true);
  tft.println(text);}

void displayMessage(String text) {
  tft.setTextColor(TFT_BLACK);
  tft.fillScreen(TFT_WHITE);
  tft.setTextFont(2);
  tft.setTextWrap(true, true);
  tft.println(text);
}

void displaySuccessMessage(String text) {
  tft.setTextColor(TFT_BLACK);
  tft.fillScreen(TFT_WHITE);
  tft.setTextFont(2);
  tft.setTextWrap(true, true);
  tft.println(text);
}

void displayInfoMessage(String text) {
  tft.setTextColor(TFT_BLACK);
  tft.fillScreen(TFT_WHITE);
  tft.setTextWrap(true, true);
  tft.println(text);
}

void drawMultilineText(String text) {
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
}