#include <Arduino.h>
#include "WiFiSetup.h"
#include "Display.h"
#include "NfcAdapter.h"
#include "PN532.h"
#include "PN532_I2C.h"

String ssidPrefix = "LNVending";
String ssid = ssidPrefix + String(ESP.getEfuseMac(), HEX);  // Unique SSID based on a prefix and the ESP32's chip ID
String password = "password123";
WiFiSetup wifiSetup(ssid.c_str(), password.c_str());

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

void displayWifiSetup(String ssid, String password, String ip);
String convertToStringFromBytes(byte dataArray[], int sizeOfArray);
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

  nfc.begin();
}

void loop() {
    wifiSetup.processDnsServerRequests();
    Serial.println("\nScan a NFC tag\n");
    if (nfc.tagPresent()) {
        NfcTag tag = nfc.read();
        tag.print();
      
      if(tag.hasNdefMessage()) {
        NdefMessage message = tag.getNdefMessage();

        // If you have more than 1 Message then it wil cycle through them
        int recordCount = message.getRecordCount();
        for(int i = 0; i < recordCount; i++) {
          Serial.println("\nNDEF Record " + String(i+1));
          NdefRecord record = message.getRecord(i);

          int payloadLength = record.getPayloadLength();
          byte payload[record.getPayloadLength()];
          record.getPayload(payload);
          String string = convertToStringFromBytes(payload, payloadLength);
          
          Serial.println("Payload Length = " + String(payloadLength));
          Serial.println("  Information (as String): " + string);
        }
      }
    }
    delay(5000);
}

String convertToStringFromBytes(byte dataArray[], int sizeOfArray) {
  String stringOfData = "";
  for(int byteIndex = 0; byteIndex < sizeOfArray; byteIndex++) {
    stringOfData += (char)dataArray[byteIndex];
  }
  return stringOfData;
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
      message = "Wi-Fi disconnected\nAttempting to reconnect...";
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
