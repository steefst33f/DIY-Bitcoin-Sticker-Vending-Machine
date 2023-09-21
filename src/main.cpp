#include <Arduino.h>

#include "WiFiSetup.h"
#include "Display.h"

void displayWifiSetup(String ssid, String password, String ip);
void onWiFiEvent(WiFiEvent_t event);
void onWifiSetupSucces(String ssid, String localIp);
void onWifiSetupFailure(String apSsid, String apPassword, String apId);

void onWifiSetupSucces(String ssid, String localIp) {
  displayWifiConnected(ssid, localIp);
}

void onWifiSetupFailure(String apSsid, String apPassword, String apId) {
  displayWifiSetup(apSSID, apPassword, apId);
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
    handleWifiEvents(onWiFiEvent);
  } else {
    startApWifiSetup(onWifiSetupSucces, onWifiSetupFailure);
    displayWifiSetup(apSSID, apPassword, apIP.toString());
  }
}

void loop() {
    processDnsServerRequests();
}

void displayWifiSetup(String ssid, String password, String ip) {
  while(isWifiStatusConnected() == false)
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
