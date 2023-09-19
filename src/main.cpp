#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>
#include <QRCode.h>  // Include the QRCode library
#include <Preferences.h>

Preferences preferences;

const String apSSID = "ESP32_AP";
const String apPassword = "password123";
IPAddress apIP(192, 168, 4, 1);

String savedSSID = "";
String savedPassword = "";

DNSServer dnsServer;
AsyncWebServer server(80);

TFT_eSPI tft = TFT_eSPI(); 

void getSavedWifiCredentials();
void setSavedWifiCredentials(String ssid, String password);
bool connectToWifi(String ssid, String password);
void startApWifiSetup();
QRCode createQrCode(String data);
void drawQrCode(QRCode qrCode, TFT_eSPI tft, int pixelSize, int xPosition, int yPosition);
void displayWifiSetupQrCode();
void displayConnectingToWifi();
void displayWifiConnected();
void drawMultilineText(String text);

void setup() {
  Serial.begin(115200);

  // Print some useful debug output - the filename and compilation time
  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

  // Setup tft screen
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE); // Clear the screen
  tft.setTextColor(TFT_BLACK);

  getSavedWifiCredentials();

  bool isConnectedToWifi = false;
  if (savedSSID != "" && savedPassword != "") {
    displayConnectingToWifi();
    isConnectedToWifi = connectToWifi(savedSSID.c_str(), savedPassword.c_str());
  } 
  
  if (isConnectedToWifi) {
    displayWifiConnected();
  } else {
    startApWifiSetup();
    displayWifiSetupQrCode();
  }
}

void loop() {
    dnsServer.processNextRequest();
}

void getSavedWifiCredentials() {
  preferences.begin("wifi-config", false);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  preferences.end();
}

void setSavedWifiCredentials(String ssid, String password) {
  preferences.begin("wifi-config", false);
  savedSSID = preferences.putString("ssid", ssid);
  savedPassword = preferences.putString("password", password);
  preferences.end();
}

bool connectToWifi(String ssid, String password) {
  // Connect to Wi-Fi
  int trials = 0;
  int maxTrials = 3;
  WiFi.begin(ssid.c_str(), password.c_str());
  while ((WiFi.status() != WL_CONNECTED)) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    trials++;
    if (trials > maxTrials) {
      Serial.println("Failed to connect to WiFi");
      return false;
    }
  }

  Serial.println("Connected to WiFi");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void startApWifiSetup() {
   // Create ESP32 AP
  WiFi.softAP(apSSID, apPassword);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  dnsServer.start(53, "*", apIP);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP().toString());
  Serial.print("AP SSID: ");
  Serial.println(apSSID);
  Serial.print("AP password: ");
  Serial.println(apPassword);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    getSavedWifiCredentials();
    // Serve the HTML page with saved credentials filled in
    String html = "<html><body>";
    html += "<h1>Wi-Fi Configuration</h1>";
    html += "<form action='/configure' method='post'>";
    html += "<label for='ssid'>SSID (Network Name):</label>";
    html += "<input type='text' id='ssid' name='ssid' placeholder='Saved SSID' value='" + savedSSID + "'>";
    html += "<br><br>";
    html += "<label for='password'>Password:</label>";
    html += "<input type='password' id='password' name='password' placeholder='Saved Password' value='" + savedPassword + "'>";
    html += "<br><br>";
    html += "<input type='submit' value='Save Configuration'>";
    html += "</form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.onNotFound([](AsyncWebServerRequest* request){
        Serial.println("**client gets redeirected to: /login ***");
        request->redirect("http://" + apIP.toString());
    });

  server.on("/configure", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    setSavedWifiCredentials(ssid, password);
    getSavedWifiCredentials();

    if (connectToWifi(savedSSID, savedPassword)) {
      request->send(200, "text/plain", "Configuration successful!");
      displayWifiConnected();
    } else {
      request->send(400, "text/plain", "Invalid configuration!");
      displayWifiSetupQrCode();
    }
  });

  server.begin();
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

void displayWifiSetupQrCode() {
  tft.fillScreen(TFT_WHITE); // Clear the screen
  tft.setTextColor(TFT_DARKCYAN);
  tft.drawString("WiFi Setup", 2, 5, 4); // Display a title
  tft.setTextColor(TFT_BLACK);
  tft.drawString("SSID: " + apSSID, 2, 30, 2); 
  tft.drawString("PassW: " + apPassword, 2, 45, 2); 
  tft.drawString("IP: " +  WiFi.softAPIP().toString(), 2, 60, 2); 
    
  // QrCode AP
  String qrCodeData;
  int qrCodePixelSize = 4;
  qrCodeData = "WIFI:S:" + apSSID + ";T:WPA2;P:" + apPassword + ";";
  QRCode qrCodeAp = createQrCode(qrCodeData);
  drawQrCode(qrCodeAp, tft, qrCodePixelSize, 2, 100);
}

void displayConnectingToWifi() {
  tft.fillScreen(TFT_WHITE); // Clear the screen
  tft.setTextPadding(tft.width()); // Remove text padding
  String longText = "Connecting to Wifi...";
  tft.drawCentreString(longText, tft.width() / 2, tft.height() / 2, 2);
}

void displayWifiConnected() {
  String wifiInfo = "Connected to WiFi\n";
  wifiInfo += "SSID: " + WiFi.SSID() + "\n";
  wifiInfo += "IP address: " + WiFi.localIP().toString() + "\n";

  tft.fillScreen(TFT_WHITE); // Clear the screen
  drawMultilineText(wifiInfo);
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

