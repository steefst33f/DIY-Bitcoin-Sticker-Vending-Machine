#include <Arduino.h>

//UnComment to show Serial debugging prints
#if SHOW_MY_DEBUG_SERIAL
#define MY_DEBUG_SERIAL Serial
#define SHOW_MY_NFC_DEBUG_SERIAL 1
#define SHOW_MY_WIFI_DEBUG_SERIAL 1
#define SHOW_MY_PAYMENT_DEBUG_SERIAL 1
#define SHOW_MY_DISPENSER_DEBUG_SERIAL 1
#endif

#include "WiFiConfiguration.h"
#include "Display.h"
// #include "NfcWrapper.h"
#include "UriComponents.h"
#include "Payment.h"
#include "Dispenser.h"

#include <NfcWrapper.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TaskScheduler.h>

String ssidPrefix = "LNVending";
String ssid = ssidPrefix + String(ESP.getEfuseMac(), HEX);  // Unique SSID based on a prefix and the ESP32's chip ID
String password = "password123";
WiFiConfiguration wifiSetup(ssid.c_str(), password.c_str());
Payment payment = Payment();

//GPIO
#define VENDOR_MODE_PIN 13
#define SERVO_PIN 15
#define FILL_DISPENSER_BUTTON 0
#define EMPTY_DISPENSER_BUTTON 35

//NFC module
#if NFC_SPI
#define PN532_SCK  (25)
#define PN532_MISO (27)
#define PN532_MOSI (26)
#define PN532_SS   (33)
Adafruit_PN532 *nfcModule = new Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
#elif NFC_I2C
#define PN532_IRQ     (2)
#define PN532_RESET   (3)
Adafruit_PN532 *nfcModule = new Adafruit_PN532(PN532_IRQ, PN532_RESET);
#else
#error "Need NFC interface defined! Define NFC_SPI=1 or NFC_I2C=1!"
#endif

NfcWrapper nfc = NfcWrapper(nfcModule);

// NfcWrapper callback handlers
void onNfcModuleConnected();
void onStartScanningTag();
void onReadingTag(/*ISO14443aTag tag*/);
void onReadTagRecord(String stringRecord);
void onFailure(NfcWrapper::Error error);

// Dispenser
Dispenser dispenser = Dispenser(VENDOR_MODE_PIN, SERVO_PIN, FILL_DISPENSER_BUTTON, EMPTY_DISPENSER_BUTTON);

// helpers
bool showQrCredentialsScreen = true;
void displayWifiSetup(String ssid, String password, String ip);
String convertToStringFromBytes(byte dataArray[], int sizeOfArray);
void onWiFiEvent(WiFiEvent_t event);

// API call
void doApiCall(String uri);

// TaskScheduler task functions
void scanningForNfc();
void checkForNewPayments();
void processingSerialStream();
void processingDnsServerRequests();
void displayWifiSetup();

// // Tasks
Scheduler taskScheduler;
Task displayWifiSetupTask(5000, TASK_FOREVER, &displayWifiSetup);
Task processingDnsServerRequestsTask(1000, TASK_FOREVER, &processingDnsServerRequests);
Task processingSerialStreamTask(TASK_IMMEDIATE, TASK_FOREVER, &processingSerialStream);
Task scanningForNfcTask(TASK_IMMEDIATE, TASK_FOREVER, &scanningForNfc);
Task checkForNewPaymentsTask(3000, TASK_FOREVER, &checkForNewPayments);


void setup() {
  Serial.begin(115200);

  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

#if NFC_SPI
  // vspi = new SPIClass(VSPI);
  pinMode(PN532_SS, OUTPUT);
  pinMode(PN532_SCK, OUTPUT);
  pinMode(PN532_MISO, INPUT);
  pinMode(PN532_MOSI, OUTPUT);

  SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
#endif
  initDisplay();

  displayLogo();
  delay(2000);

  dispenser.begin();

#if !DEMO
  // dispenser.waitForVendorMode(3000);
#endif

#if WIFI
  wifiSetup.begin();

  //If not setup yet display Portal Access Credentials
  //Waiting for user to configure Wifi via Portal, will show Portal Access Credentials (Alterneting between QR and text)
  taskScheduler.addTask(processingDnsServerRequestsTask);
  taskScheduler.addTask(displayWifiSetupTask);
  taskScheduler.addTask(processingSerialStreamTask);


  processingDnsServerRequestsTask.enable();
  displayWifiSetupTask.enable();
  processingSerialStreamTask.enable();
  
  while(!wifiSetup.isWifiStatusConnected())
  {
    taskScheduler.execute();
  } 

  taskScheduler.deleteTask(displayWifiSetupTask);
  taskScheduler.deleteTask(processingDnsServerRequestsTask);
  taskScheduler.deleteTask(processingSerialStreamTask);

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
  nfc.setOnStartScanningForTag(onStartScanningTag);
  nfc.setOnReadMessageRecord(onReadTagRecord);
  nfc.setOnStartReadingTag(onReadingTag);
  nfc.setOnFailure(onFailure);
  nfc.begin();

  // TaskScheduler
  taskScheduler.addTask(scanningForNfcTask);
  taskScheduler.addTask(checkForNewPaymentsTask);

  scanningForNfcTask.enable();
  checkForNewPaymentsTask.enable();
}

void loop() {
  taskScheduler.execute();
  delay(1000);  
}

//////////////////task functions//////////////////

void scanningForNfc() {
  // log_e();
  // Serial.println("scanningForNfc");
  if (nfc.isNfcModuleAvailable()) {
    nfc.scanForTag();
  }
}

void checkForNewPayments() {
  log_e();
  if (payment.hasReceivedNewPayment(payment.getVendingPrice())) {
    displayPayed(String(payment.getVendingPrice()));
    dispenser.dispense();
  }
}

void processingSerialStream() {
  handleIncommingStream();
}

void processingDnsServerRequests() {
  log_e();
  wifiSetup.processDnsServerRequests();
}

void displayWifiSetup() {
  String portalSsid = wifiSetup.getPortalSsid();
  String portalPassword = wifiSetup.getPortalPassword();
  String portalIp = wifiSetup.getPortalIp();
  String qrCredentials = "WIFI:S:" + portalSsid + ";T:WPA2;P:" + password + ";";

  if (showQrCredentialsScreen) {
    displayWifiSetupQrCode(qrCredentials);
  } else {
    displayWifiCredentials(portalSsid, portalPassword, portalIp);
  }
  showQrCredentialsScreen = !showQrCredentialsScreen;
}

#if WIFI
// Wifi events handler
void onWiFiEvent(WiFiEvent_t event) {
  String message = "";
  String localIp = wifiSetup.getLocalIp();
  String ssid = wifiSetup.getConfiguredSsid();
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      message = "Connected to Wi-Fi, IP address: " + localIp;
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayWifiConnected(ssid, localIp);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      message = "Wi-Fi disconnected\nAttempting to reconnect...";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayErrorScreen("Wifi Error", message);
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_WIFI_READY:
      message = "Wi-Fi interface ready";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi Ready", message);
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      message = "Wi-Fi scan completed";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_START:
      message = "Station mode started";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_STOP:
      message = "Station mode stopped";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      message = "Connected to AP";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      message = "Authentication mode changed";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      message = "Lost IP address";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      message = "WPS success in station mode";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      message = "WPS failed in station mode";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      message = "WPS timeout in station mode";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      message = "WPS pin code in station mode";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_START:
      message = "Access Point started";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STOP:
      message = "Access Point stopped";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      message = "Station connected to Access Point";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      message = "Station disconnected from Access Point";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
    default:
      message = "Unknown Wi-Fi event";
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println(message);
      #endif
      displayScreen("Wifi", message);
      break;
  }
}
#endif

//NFC callback handlers
void onNfcModuleConnected() {
  displayScreen("NFC Ready!", "LNURLWithdraw  NFC payment is available");
}

void onStartScanningTag() {
  displayPriceToPay(String(payment.getVendingPrice()));
}

void onReadingTag(/*ISO14443aTag tag*/) {
  displayScreen("Reading NFC Card..", "Don't remove the card untill done");
}

void onReadTagRecord(String stringRecord) {
  displayScreen("Read NFC record:", stringRecord);
  payment.payWithLnUrlWithdrawl(stringRecord);
}

void onFailure(NfcWrapper::Error error) {
  if (error == NfcWrapper::Error::scanFailed) {
      debugDisplayText("No NFC card detected yet..");
      return;
  }

  String errorString;
  switch (error) {
    case NfcWrapper::Error::connectionModuleFailed:
      errorString = "Not connected to NFC module";
      break;
    case NfcWrapper::Error::scanFailed:
      errorString = "Failed ";
      break;
    case NfcWrapper::Error::readFailed:
      errorString = "Failed to read card";
      break;
    case NfcWrapper::Error::identifyFailed:
      errorString = "Couldn't identify tag";
      break;
    case NfcWrapper::Error::releaseFailed:
      errorString = "Failed to release tag properly";
      break;
    default:
      errorString = "unknow error";
      break;
  }
  displayErrorScreen("NFC payment Error", errorString);

  if (error == NfcWrapper::Error::connectionModuleFailed) {
    delay(2000);
    nfc.powerDownMode();
    nfc.begin();
  }
}

// API call
void doApiCall(String uri) {
  #ifdef MY_DEBUG_SERIAL
  MY_DEBUG_SERIAL.println("uri: " + uri);
  #endif
  WiFiClientSecure client;
  client.setInsecure();
  UriComponents uriComponents = UriComponents::Parse(uri.c_str());
  String host = uriComponents.host.c_str();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

      // Your API endpoint
    http.begin(uri);

    // Send GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println("HTTP Response Code: " + String(httpResponseCode));
      MY_DEBUG_SERIAL.println("Response Data: " + payload);
      #endif

      // Parse JSON response
      DynamicJsonDocument jsonDoc(1024); // Adjust the buffer size as needed
      DeserializationError jsonError = deserializeJson(jsonDoc, payload);

      if (jsonError) {
        #ifdef MY_DEBUG_SERIAL
        MY_DEBUG_SERIAL.println("JSON parsing error: " + String(jsonError.c_str()));
        #endif
      } else {
        // Access JSON properties
        String name = jsonDoc["name"];
        String lastName = jsonDoc["lastname"];
        int age = jsonDoc["age"];

        #ifdef MY_DEBUG_SERIAL
        MY_DEBUG_SERIAL.println("Name: " + name);
        MY_DEBUG_SERIAL.println("Last Name: " + lastName);
        MY_DEBUG_SERIAL.println("Age: " + String(age));
        #endif
      }
    } else {
      #ifdef MY_DEBUG_SERIAL
      MY_DEBUG_SERIAL.println("Error in HTTP request");
      #endif
    }

    http.end();
  }
}
