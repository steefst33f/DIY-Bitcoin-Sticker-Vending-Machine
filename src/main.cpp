#include <Arduino.h>
#include "WiFiSetup.h"
#include "Display.h"
#include "NfcAdapter.h"
#include "PN532.h"
#include "PN532_I2C.h"
#include "UriComponents.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

struct Withdrawal {
    String tag;
    String callback;
    String k1;
    int minWithdrawable;
    int maxWithdrawable;
    String defaultDescription;
};

struct Invoice {
  String paymentHash;
  String paymentRequest;
  String checkingId;
  String lnurlResponse;
};

bool down = false;
bool paid = false;

// Configurable variables
String amount = "210"; //Amount in Sats
String lnbitsServer = "legend.lnbits.com";
String lnurlP = "LNURL1DP68GURN8GHJ7MR9VAJKUEPWD3HXY6T5WVHXXMMD9AKXUATJD3CZ7CTSDYHHVVF0D3H82UNV9U6RVVPK5TYPNE";
String invoiceKey = "4b76adae4f1a4dc38f93e892a8fba8b2";

String dataId = "";
String description = "";
String payReq = "";

String ssidPrefix = "LNVending";
String ssid = ssidPrefix + String(ESP.getEfuseMac(), HEX);  // Unique SSID based on a prefix and the ESP32's chip ID
String password = "password123";
WiFiSetup wifiSetup(ssid.c_str(), password.c_str());

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

void displayWifiSetup(String ssid, String password, String ip);
String scannedLnUrlFromNfcTag();
String decode(String lnUrl);
String getLNURL(const String string);
String convertToStringFromBytes(byte dataArray[], int sizeOfArray);
void onWiFiEvent(WiFiEvent_t event);

bool withdraw(String callback, String k1, String pr);
Withdrawal getWithdrawal(String uri);
bool isAmountInWithdrawableBounds(int amount, int minWithdrawable, int maxWithdrawable);
Invoice getInvoice(String description);
bool checkInvoice(String invoiceId);


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
    String lnUrl = scannedLnUrlFromNfcTag();
    String decodedLnUrl = decode(lnUrl);
    Withdrawal withdrawal = getWithdrawal(decodedLnUrl);

    if(withdrawal.tag != "withdrawRequest") {
      Serial.println(F("Scanned tag is not LNURL withdraw"));
      Serial.println(F("Present a tag with a LNURL withdraw on it"));
      // debugDisplayText(F("Scanned tag is not LNURL withdraw \r\nPresent a tag with a LNURL withdraw on it"));
      return;
    }

    Serial.println(F("Scanned tag is a LNURL withdrawal request!"));
    if(!isAmountInWithdrawableBounds(amount.toInt(), withdrawal.minWithdrawable,  withdrawal.maxWithdrawable)) {
      Serial.println("The requested amount: " + amount + " is not within this LNURL withdrawal bounds");
      Serial.println(F("Amount not in bounds, can't withdraw from presented voucher."));
      // debugDisplayText(F("Amount not in bounds, can't withdraw from presented voucher."));
      return;
    }

    Serial.println("The requested amount: " + amount);
    Serial.println(F(" is within this LNURL withdrawal bounds"));
    Serial.println(F("Continue payment flow by creating invoice"));

    Serial.println(F("Will create invoice to request withrawal..."));
    // debugDisplayText("Creating invoice..");
    Invoice invoice = getInvoice("BitcoinSwitch QR");
    Serial.println("invoice.paymentHash = " + invoice.paymentHash); 
    Serial.println("invoice.paymentRequest = " + invoice.paymentRequest);
    Serial.println("invoice.checkingId = " + invoice.checkingId);  
    Serial.println("invoice.lnurlResponse = " + invoice.lnurlResponse); 
    Serial.println(F(""));

    if(invoice.paymentRequest == "") {
      Serial.println(F("Failed to create invoice"));
      // debugDisplayText(F("Failed to create invoice")); 
      return;
    }

    bool success = withdraw(withdrawal.callback, withdrawal.k1, invoice.paymentRequest);
    if(!success) {
      Serial.println(F("Failed to request withdrawalfor invoice request with memo: ")); // + invoice.memo);
      // debugDisplayText(F("Failed to request withdrawal invoice"));
    } else {
      Serial.println(F("Withdrawal request successfull!"));
      //TODO: Check if open invoice is paid
      bool isPaid = checkInvoice(invoice.checkingId);

      int numberOfTries = 1;
      while(!isPaid && (numberOfTries < 3)) {
        delay(2000);
        isPaid = checkInvoice(invoice.checkingId);
        numberOfTries++;
      }

      if(!isPaid) {
        Serial.println(F("Could not confirm withdrawal, the invoice has not been payed in time"));
        // debugDisplayText("Could not confirm withdrawal, transaction cancelled");
        return;
      }
      Serial.println(F("Withdrawal successfull, invoice is payed!"));
      // debugDisplayText(F("Withdrawal succeeded!! Thank you!"));
      paid = true;
    }

    delay(5000);  
}

String decode(String lnUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  down = false;

  if(!client.connect(lnbitsServer.c_str(), 443)) {
    Serial.println("Client couldn't connect to LNBitsServer to decode LNURL");
    down = true;
    return "";   
  }

  String body = "{\"data\": \"" + lnUrl + "\"}";
  String url = "/api/v1/payments/decode";
  String request = String("POST ") + url + " HTTP/1.1\r\n" +
                "Host: " + lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + body.length() + "\r\n" +
                "\r\n" + 
                body + "\n";
  client.print(request);
  Serial.println(request);
   
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  const size_t capacity = JSON_OBJECT_SIZE(2) + 800;
  DynamicJsonDocument doc(capacity);  
  
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    Serial.println("deserializeJson() failed: " + String(error.f_str()));
    return "";
  }
  
  const char* domain = doc["domain"]; 
  Serial.println("decodedLNURL: " + String(domain));
  return domain;
}

String scannedLnUrlFromNfcTag() {
  Serial.println("\nScan a NFC tag\n");
  if (nfc.tagPresent()) {
    Serial.println("Found Tag!");
    NfcTag tag = nfc.read();
    tag.print();
    
    if(tag.hasNdefMessage()) {
      Serial.println("Found NDEF Message!");
      NdefMessage message = tag.getNdefMessage();

      // If more than 1 Message then it wil cycle through them untill it finds a LNURL
      int recordCount = message.getRecordCount();
      for(int i = 0; i < recordCount; i++) {
        Serial.println("\nNDEF Record " + String(i+1));
        NdefRecord record = message.getRecord(i);

        int payloadLength = record.getPayloadLength();
        byte payload[record.getPayloadLength()];
        record.getPayload(payload);
        //+1 to skip first byte of payload, which is always null
        String stringRecord = convertToStringFromBytes(payload+1, payloadLength-1);
        
        Serial.println("Payload Length = " + String(payloadLength));
        Serial.println("  Information (as String): " + stringRecord);

        String lnUrl = getLNURL(stringRecord);
        if (lnUrl != "") {
          Serial.println("Found LNURL!");
          return lnUrl;
        }
      }
    }
  }
  return "";
}

/// LNURLw functions
String getLNURL(const String string) {
  Serial.println("bool islLNURL(" + string + ")");
  String editedString = string;

  if(string.startsWith("lnurlw://")) {
    Serial.println("found lightning URI with LNURL: " + string);
    editedString.remove(0,8);
    Serial.println(string);
    return editedString;
  }
 
  editedString.trim();
  editedString.toUpperCase();

  if(editedString.startsWith("LNURL")) {
    Serial.println(F("found LNURL"));
    return editedString;
  } else if(editedString.startsWith("LIGHTNING:LNURL")) {
    Serial.println("found lightning URI with LNURL: " + editedString);
    editedString.remove(0,10);
    return editedString;     
  } else {
    Serial.println(F("string is no LNURL"));
    return "";
  }
}

String convertToStringFromBytes(byte dataArray[], int sizeOfArray) {
  String stringOfData = "";
  for(int byteIndex = 0; byteIndex < sizeOfArray; byteIndex++) {
    stringOfData += (char)dataArray[byteIndex];
  }
  return stringOfData;


  // // Convert the byte array to a char array
  // char charArray[sizeOfArray + 1];  // +1 to leave space for the null terminator
  // for (int i = 0; i < sizeOfArray; i++) {
  //   charArray[i] = (char)dataArray[i];
  // }
  // charArray[sizeOfArray] = '\0';  // Null-terminate the char array

  // // Convert the char array to an Arduino String
  // return String(charArray);
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

Withdrawal getWithdrawal(String uri) {
  WiFiClientSecure client;
  client.setInsecure();
  down = false;
  UriComponents uriComponents = UriComponents::Parse(uri.c_str());
  String host = uriComponents.host.c_str();

  if(!client.connect(host.c_str(), 443)) {
    Serial.println("Client couldn't connect to service to get Withdrawl");
    down = true;
    return {};   
  }
  
  Serial.println(F("xxxxx!!!!!!"));
  String request = String("GET ") + uri + " HTTP/1.0\r\n" +
    "User-Agent: ESP32\r\n" +
    "accept: text/html\r\n" +
    "Connection: close\r\n\r\n";
  Serial.println(request);
  client.print(request);

  while(client.connected()) {
   String line = client.readStringUntil('\n');
   if(line == "\r") {
     break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  // debugDisplayText(line);
  
  const size_t capacity = JSON_OBJECT_SIZE(2) + 800;
  DynamicJsonDocument doc(capacity);  
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    Serial.println(("deserializeJson() failed: ") + String(error.f_str()));
    return {};
  }

  return {
    doc["tag"],
    doc["callback"],
    doc["k1"],
    doc["minWithdrawable"],
    doc["maxWithdrawable"],
    doc["defaultDescription"]
  };
}

bool isAmountInWithdrawableBounds(int amount, int minWithdrawable, int maxWithdrawable) {
  int amountInMilliSats = amount * 1000;
  Serial.println("((" + String(amountInMilliSats) + " >= " + String(minWithdrawable) + ") && (" + String(amountInMilliSats) + " <= " + String(maxWithdrawable) + "))");
  return ((amountInMilliSats >=minWithdrawable) && (amountInMilliSats <= maxWithdrawable));
}

Invoice getInvoice(String description) 
{
  WiFiClientSecure client;
  client.setInsecure();
  down = false;

  if(!client.connect(lnbitsServer.c_str(), 443)) {
    Serial.println("Client couldn't connect to LNBitsServer to create Invoice");
    down = true;
    return {};   
  }

  String topost = "{\"out\": false,\"amount\" : " + String(amount) + ", \"memo\" :\""+ String(description) + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  String request = (String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ invoiceKey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");
  client.print(request);
  Serial.println(request);
  
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  
  String line = client.readString();
  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    Serial.println("deserializeJson() failed: " + String(error.f_str()));
    return {};
  }

  //TODO: remove these user struct instsead
  const char* payment_hash = doc["checking_id"];
  const char* payment_request = doc["payment_request"];
  payReq = payment_request;
  dataId = payment_hash;

  return {
    doc["payment_hash"],
    doc["payment_request"],
    doc["checking_id"],
    doc["lnurl_response"]
  };
}

bool withdraw(String callback, String k1, String pr) {
  WiFiClientSecure client;
  client.setInsecure();
  UriComponents uriComponents = UriComponents::Parse(callback.c_str());
  String host = uriComponents.host.c_str();
  down = false;

  if(!client.connect(host.c_str(), 443)) {
    down = true;
    return {};   
  }
  String requestParameters = ("k1=" + k1 + "&pr=" + pr);
  String request = (String("GET ") + callback + "?" + requestParameters + " HTTP/1.0\r\n" +
                "User-Agent: ESP32\r\n" +
                "accept: text/plain\r\n" +
                "Connection: close\r\n\r\n");
  client.print(request);
  Serial.println(request);
  
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    Serial.println("deserializeJson() failed: " + String(error.f_str()));
    return false;
  }
  bool isOk = doc["status"];
  return isOk;
}

bool checkInvoice(String invoiceId) {
  WiFiClientSecure client;
  client.setInsecure();
  down = false;

  if(!client.connect(lnbitsServer.c_str(), 443)) {
    Serial.println("Client couldn't connect to LNBitsServer to check Invoice");
    down = true;
    return false;   
  }

  String url = "/api/v1/payments/";
  String request = (String("GET ") + url + invoiceId +" HTTP/1.1\r\n" +
                "Host: " + lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
  client.print(request);
  Serial.println(request);
  
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    Serial.println("deserializeJson() failed: " + String(error.f_str()));
    return false;
  }
  bool isPaid = doc["paid"];
  return isPaid;
}
