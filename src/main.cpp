#include <Arduino.h>
#include "WiFiConfiguration.h"
#include "Display.h"
#include "Nfc.h"
#include "UriComponents.h"
#include "Payment.h"
#include "Dispenser.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#if NFC_SPI
#include <SPI.h>
#endif

String ssidPrefix = "LNVending";
String ssid = ssidPrefix + String(ESP.getEfuseMac(), HEX);  // Unique SSID based on a prefix and the ESP32's chip ID
String password = "password123";
WiFiConfiguration wifiSetup(ssid.c_str(), password.c_str());
Payment payment = Payment();

//GPIO
#define VENDOR_MODE_PIN 33
#define SERVO_PIN 27
#define FILL_DISPENSER_BUTTON 0
#define EMPTY_DISPENSER_BUTTON 35

#if NFC_SPI
#define PN532_SCK  (2)
#define PN532_MISO (15)
#define PN532_MOSI (13)
#define PN532_SS   (37)
#endif
// #define DEMO 1

Nfc nfc = Nfc();
Dispenser dispenser = Dispenser(VENDOR_MODE_PIN, SERVO_PIN, FILL_DISPENSER_BUTTON, EMPTY_DISPENSER_BUTTON);

void displayWifiSetup(String ssid, String password, String ip);
// String scannedLnUrlFromNfcTag();
String convertToStringFromBytes(byte dataArray[], int sizeOfArray);
void onWiFiEvent(WiFiEvent_t event);

// Nfc callback handlers
void onNfcModuleConnected();
void onStartScanningTag();
void onReadingTag(/*ISO14443aTag tag*/);
void onReadTagRecord(String stringRecord);
void onFailure(Nfc::Error error);

void setup() {
  Serial.begin(115200);

  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

#if NFC_SPI
  SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
#endif
  initDisplay();

  displayLogo();
  delay(2000);

  dispenser.begin();

  #if !DEMO
  dispenser.waitForVendorMode(3000);

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

  #endif

  String amount = wifiSetup.getConfiguredAmount();
  String lnbitsServer = wifiSetup.getConfiguredLnbitsServer();
  String invoiceKey = wifiSetup.getConfiguredInvoiceKey();
   
  payment.configure(amount.c_str(), lnbitsServer.c_str(), invoiceKey.c_str());

  //setup nfc callback handlers
  nfc.setOnNfcModuleConnected(onNfcModuleConnected);
  nfc.setOnStartScanningTag(onStartScanningTag);
  nfc.setOnReadMessageRecord(onReadTagRecord);
  nfc.setOnReadingTag(onReadingTag);
  nfc.setOnFailure(onFailure);
  nfc.begin();
}

void loop() {
    // wifiSetup.processDnsServerRequests();
    nfc.powerDownMode();
    nfc.begin();
    if (nfc.isNfcModuleAvailable()) {
      nfc.scanForTag();
    }
    delay(5000);  
}

// Wifi events handler
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
      displayErrorScreen("Wifi Error", message);
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_WIFI_READY:
      message = "Wi-Fi interface ready";
      Serial.println(message);
      displayScreen("Wifi Ready", message);
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      message = "Wi-Fi scan completed";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_START:
      message = "Station mode started";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_STOP:
      message = "Station mode stopped";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      message = "Connected to AP";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      message = "Authentication mode changed";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      message = "Lost IP address";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      message = "WPS success in station mode";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      message = "WPS failed in station mode";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      message = "WPS timeout in station mode";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      message = "WPS pin code in station mode";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_START:
      message = "Access Point started";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STOP:
      message = "Access Point stopped";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      message = "Station connected to Access Point";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      message = "Station disconnected from Access Point";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
    default:
      message = "Unknown Wi-Fi event";
      Serial.println(message);
      displayScreen("Wifi", message);
      break;
  }
}

//NFC callback handlers
void onNfcModuleConnected() {
  displayScreen("NFC Ready!", "LNURLWithdraw  NFC payment is available");
}

void onStartScanningTag() {
  displayPriceToPay(String(payment.getVendingPrice()));
}

void onReadingTag(/*ISO14443aTag tag*/) {
  displayScreen("Reading NFC Card..", "Don't remove the card untill done");
  // tag.print();
}

void onReadTagRecord(String stringRecord) {
  displayScreen("Read NFC record:", stringRecord);
  if(payment.payWithLnUrlWithdrawl(stringRecord)) {
    displayPayed(String(payment.getVendingPrice()));
    dispenser.dispense();
  }
}

void onFailure(Nfc::Error error) {
  if (error == Nfc::Error::scanFailed) {
      debugDisplayText("No NFC card detected yet..");
      return;
  }

  String errorString;
  switch (error) {
    case Nfc::Error::connectionModuleFailed:
      errorString = "Not connected to NFC module";
      break;
    case Nfc::Error::scanFailed:
      errorString = "Failed ";
      break;
    case Nfc::Error::readFailed:
      errorString = "Failed to read card";
      break;
    case Nfc::Error::identifyFailed:
      errorString = "Couldn't identify tag";
      break;
    case Nfc::Error::releaseFailed:
      errorString = "Failed to release tag properly";
      break;
    default:
      errorString = "unknow error";
      break;
  }
  displayErrorScreen("NFC payment Error", errorString);

  if (error == Nfc::Error::connectionModuleFailed) {
    delay(2000);
    nfc.powerDownMode();
    nfc.begin();
  }
}

