#include <Arduino.h>
#include "WiFiSetup.h"
#include "Display.h"
#include "Nfc.h"
#include "UriComponents.h"
#include "Payment.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <ESP32Servo.h>

//GPIO
int servoPin = 27;
int vendorModePin = 33;
int fillDispencerButton = 0;
int emptyDispencerButton = 35;

String ssidPrefix = "LNVending";
String ssid = ssidPrefix + String(ESP.getEfuseMac(), HEX);  // Unique SSID based on a prefix and the ESP32's chip ID
String password = "password123";
WiFiSetup wifiSetup(ssid.c_str(), password.c_str());

Nfc nfc = Nfc();
Servo servo;

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

void dispense();
void fillDispenser();
void emptyDispenser();

void setup() {
  Serial.begin(115200);

  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

  initDisplay();
  
  //setup dispenser
  servo.attach(servoPin);
  pinMode(fillDispencerButton, INPUT);
  pinMode(emptyDispencerButton, INPUT);

  int timer = 0;
  while(timer < 2000) {
    //Setup vendor mode
    Serial.println("vendorPin: " + String(touchRead(vendorModePin)));
    if(touchRead(vendorModePin) < 50) {
      Serial.println(F("In Vendor Fill/Empty mode"));
      Serial.println(F("(Restart Vending Machine to exit)"));
      displayVendorMode();

      ////Dispenser buttons scanning////
      while (true) {
        if (digitalRead(fillDispencerButton) == LOW) {
          fillDispenser();
        } else if(digitalRead(emptyDispencerButton) == LOW) {
          emptyDispenser();
        }
        delay(500);
      }  
    }
    timer = timer + 100;
    delay(300);
  }

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

  //setup nfc callback handlers
  nfc.setOnNfcModuleConnected(onNfcModuleConnected);
  nfc.setOnStartScanningTag(onStartScanningTag);
  nfc.setOnReadMessageRecord(onReadTagRecord);
  nfc.setOnReadingTag(onReadingTag);
  nfc.setOnFailure(onFailure);
  nfc.begin();
}

void loop() {
    wifiSetup.processDnsServerRequests();
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

/// Servo Code
//////// Servo ///////
void dispense() {
  Serial.println(F("Vending Machine dispense START!"));
  servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Vending Machine dispense STOP!!"));
}

void fillDispenser() {
  Serial.println(F("Fill dispenser!!"));
  // let dispencer slowly turn in the fillup direction, so the vender can empty the dispendser with new products:
  servo.writeMicroseconds(2000); // rotate counter clockwise (from the buyers point of view)
  delay(1590);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Done!"));
}

void emptyDispenser() {
  Serial.println(F("Empty dispenser!!"));
  // let dispencer slowly turn in dispence direction, so the vender can empty all products from dispenser:
  servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Done!"));
}

//NFC callback handlers
void onNfcModuleConnected() {
  debugDisplayText("NFC payment available!");
}

void onStartScanningTag() {
  debugDisplayText("Scanning for NFC Card..");
}

void onReadingTag(/*ISO14443aTag tag*/) {
  debugDisplayText("Reading NFC Card..");
  // tag.print();
}

void onReadTagRecord(String stringRecord) {
  debugDisplayText("Read NFC record: \n" + stringRecord);
  if(payWithLnUrlWithdrawl(stringRecord)) {
    dispense();
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
      errorString = "readFailed";
      break;
    case Nfc::Error::identifyFailed:
      errorString = "identifyFailed";
      break;
    case Nfc::Error::releaseFailed:
      errorString = "releaseFailed";
      break;
    default:
      errorString = "unknow error";
      break;
  }
  setDisplayErrorText("NFC payment Failed!\n Error: !" + errorString);
}

