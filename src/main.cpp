#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "Display.h"

Preferences preferences;

const String apSSID = "ESP32_AP";
const String apPassword = "password123";
IPAddress apIP(192, 168, 4, 1);

String savedSSID = "";
String savedPassword = "";

DNSServer dnsServer;
AsyncWebServer server(80);

void getSavedWifiCredentials();
void setSavedWifiCredentials(String ssid, String password);
bool connectToWifi(String ssid, String password);
void startApWifiSetup();
void displayWifiSetup(String ssid, String password, String ip);

void onWiFiEvent(WiFiEvent_t event);

String localIp() {
  return WiFi.localIP().toString();
}

void setup() {
  Serial.begin(115200);

  // Print some useful debug output - the filename and compilation time
  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

  initDisplay();

  getSavedWifiCredentials();

  bool isConnectedToWifi = false;
  if (savedSSID != "" && savedPassword != "") {
    displayConnectingToWifi();
    isConnectedToWifi = connectToWifi(savedSSID.c_str(), savedPassword.c_str());
  } 
  
  if (isConnectedToWifi) {
    displayWifiConnected(savedSSID, localIp());
    WiFi.onEvent(onWiFiEvent);
  } else {
    startApWifiSetup();
    displayWifiSetup(apSSID, apPassword, apIP.toString());
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
  Serial.println(localIp());
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
      displayWifiConnected(ssid, localIp());
    } else {
      request->send(400, "text/plain", "Invalid configuration!");
      displayWifiSetup(apSSID, apPassword, apIP.toString());
    }
  });

  server.begin();
}

void displayWifiSetup(String ssid, String password, String ip) {
  while(WiFi.status() != WL_CONNECTED)
  {
    displayWifiCredentials(ssid, password, ip);
    delay(7000);
    displayWifiSetupQrCode("WIFI:S:" + ssid + ";T:WPA2;P:" + password + ";");
    delay(7000);
  }
}

void onWiFiEvent(WiFiEvent_t event) {
  String message = "";
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      message = "Connected to Wi-Fi, IP address: " + localIp();
      Serial.println(message);
      displayWifiConnected(savedSSID, localIp());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      message = "Wi-Fi disconnected, attempting to reconnect...";
      Serial.println(message);
      displayErrorMessage(message);
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_WIFI_READY:
      message = "Wi-Fi interface ready";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      message = "Wi-Fi scan completed";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_START:
      message = "Station mode started";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_STOP:
      message = "Station mode stopped";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      message = "Connected to AP";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      message = "Authentication mode changed";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      message = "Lost IP address";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      message = "WPS success in station mode";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      message = "WPS failed in station mode";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      message = "WPS timeout in station mode";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      message = "WPS pin code in station mode";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_AP_START:
      message = "Access Point started";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_AP_STOP:
      message = "Access Point stopped";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      message = "Station connected to Access Point";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      message = "Station disconnected from Access Point";
      Serial.println(message);
      displayInfoMessage(message);
      break;
    default:
      message = "Unknown Wi-Fi event";
      Serial.println(message);
      displayInfoMessage(message);
      break;
  }
}
