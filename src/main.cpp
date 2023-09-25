#include <Arduino.h>
#include "WiFiSetup.h"
#include "Display.h"

const char* ssid = "ESP32_AP";
const char* password = "password123";
WiFiSetup wifiSetup(ssid, password);

void displayWifiSetup(String ssid, String password, String ip);
void onWiFiEvent(WiFiEvent_t event);

void setup() {
  Serial.begin(115200);

  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

  initDisplay();

  wifiSetup.begin();
  String portalSsid = wifiSetup.getPortalSsid();
  String portalPassword = wifiSetup.getPortalPassword();
  String portalIp = wifiSetup.getPortalIp();
  String qrCredentials = "WIFI:S:" + portalSsid + ";T:WPA2;P:" + password + ";";

  //If not setup yet display Portal Access Credentials
  //Waiting for user to configure Wifi via Portal, will show Portal Access Credentials (Alterneting between QR and text)
  while(!wifiSetup.isWifiStatusConnected())
  {
    displayWifiCredentials(portalSsid, portalPassword, portalIp);
    delay(7000);
    displayWifiSetupQrCode(qrCredentials);
    delay(7000);
  } 

//Once connected start handling Wifi events and display connected
displayWifiConnected(wifiSetup.getConfiguredSsid(), wifiSetup.getLocalIp());
wifiSetup.handleWifiEvents(onWiFiEvent);
}

void loop() {
    wifiSetup.processDnsServerRequests();
}

void onWiFiEvent(WiFiEvent_t event) {
  String message = "";
  String localIp = wifiSetup.getLocalIp();
  String ssid = wifiSetup.getConfiguredSsid();
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      message = "Connected to Wi-Fi, IP address: " + localIp;
      Serial.println(message);
      displayWifiConnected(ssid, localIp);
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
