#include "Payment.h"
#include "Display.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "UriComponents.h"

String amount = "210"; //Amount in Sats
String lnbitsServer = "legend.lnbits.com";
String lnurlP = "LNURL1DP68GURN8GHJ7MR9VAJKUEPWD3HXY6T5WVHXXMMD9AKXUATJD3CZ7CTSDYHHVVF0D3H82UNV9U6RVVPK5TYPNE";
String invoiceKey = "4b76adae4f1a4dc38f93e892a8fba8b2";

String dataId = "";
String description = "";
String payReq = "";

bool down = false;
bool paid = false;

int getVendingPrice() {
  return amount.toInt();
}

bool payWithLnUrlWithdrawl(String url) {
  String lnUrl = getUrl(url);
  if (lnUrl == "") { return false; }

  Withdrawal withdrawal = getWithdrawal(lnUrl);

  if(withdrawal.tag != "withdrawRequest") {
    Serial.println(F("Scanned tag is not LNURL withdraw"));
    Serial.println(F("Present a tag with a LNURL withdraw on it"));
    displayErrorScreen(F("Withdrawal Failure"),F("Scanned tag is not LNURL withdraw \r\nPresent a tag with a LNURL withdraw on it"));
    return false;
  }

  Serial.println(F("Scanned tag is a LNURL withdrawal request!"));
  if(!isAmountInWithdrawableBounds(amount.toInt(), withdrawal.minWithdrawable,  withdrawal.maxWithdrawable)) {
    Serial.println("The requested amount: " + amount + " is not within this LNURL withdrawal bounds");
    Serial.println(F("Amount not in bounds, can't withdraw from presented voucher."));
    displayErrorScreen(F("Withdrawal Failure"), "Amount not in bounds.\n Card only alows amounts between: " + String(withdrawal.minWithdrawable) + " - " + String(withdrawal.maxWithdrawable) + " sats");
    return false;
  }

  Serial.println("The requested amount: " + amount);
  Serial.println(F(" is within this LNURL withdrawal bounds"));
  Serial.println(F("Continue payment flow by creating invoice"));

  Serial.println(F("Will create invoice to request withrawal..."));
  displayScreen("", "Creating invoice..");
  Invoice invoice = getInvoice("BitcoinSwitch QR");
  Serial.println("invoice.paymentHash = " + invoice.paymentHash); 
  Serial.println("invoice.paymentRequest = " + invoice.paymentRequest);
  Serial.println("invoice.checkingId = " + invoice.checkingId);  
  Serial.println("invoice.lnurlResponse = " + invoice.lnurlResponse); 
  Serial.println(F(""));

  if(invoice.paymentRequest == "") {
    Serial.println(F("Failed to create invoice"));
    displayErrorScreen(F("Withdrawal Failure"), F("Failed to create invoice")); 
    return false;
  }

  displayScreen("", F("Requesting withdrawal..")); 

  bool success = withdraw(withdrawal.callback, withdrawal.k1, invoice.paymentRequest);
  if(!success) {
    Serial.println(F("Failed to request withdrawalfor invoice request with memo: ")); // + invoice.memo);
    displayErrorScreen(F("Withdrawal Failure"), ("Failed to request withdrawal invoice"));
    return false;
  } 

  Serial.println(F("Withdrawal request successfull!"));
  //TODO: Check if open invoice is paid
  bool isPaid = checkInvoice(invoice.checkingId);

  int numberOfTries = 1;
  displayScreen("", F("Waiting for payment confirmation..")); 
  while(!isPaid && (numberOfTries < 5)) {
    delay(2000);
    isPaid = checkInvoice(invoice.checkingId);
    numberOfTries++;
  }

  if(!isPaid) {
    Serial.println(F("Could not confirm withdrawal, the invoice has not been payed in time"));
    displayErrorScreen(F("Withdrawal Failure"), F("Could not confirm withdrawal, transaction cancelled"));
    return false;
  }

  Serial.println(F("Withdrawal successfull, invoice is payed!"));
  displayScreen("", F("Withdrawal succeeded!! Thank you!"));
  return true;
}

Withdrawal getWithdrawal(String uri) {
  Serial.println("uri: " + uri);
  WiFiClientSecure client;
  client.setInsecure();
  down = false;
  UriComponents uriComponents = UriComponents::Parse(uri.c_str());
  String host = uriComponents.host.c_str();

  Serial.println("host: " + host);

  if(!client.connect(host.c_str(), 443)) {
    Serial.println("Client couldn't connect to service: " + host + " to get Withdrawl");
    down = true;
    // return {};   
  }
  
  String request = String("GET ") + uri + " HTTP/1.0\r\n" +
    "User-Agent: ESP32\r\n" +
    "accept: text/html\r\n" +
    "Host: " + host + "\r\n\r\n";
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
                "Host: " + host + "\r\n\r\n");
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

String getUrl(String string) {
  Serial.println(string);

  //Find the index of "://"
  int index = string.indexOf("://");
  
  // If "://" is found remove the uri from the string
  if (index != -1) {
    Serial.println(string.substring(0,index));
    string.remove(0,index + 3);
    Serial.println(string);
  }

  // If the string starts with "Lightning:"" remove it
  String uppercaseString = string;
  uppercaseString.toUpperCase();
  if (uppercaseString.startsWith("LIGHTNING:")) {
      
    Serial.println("string.startsWith(LIGHTNING:");
    Serial.println(string);
    string.remove(0,10);
    Serial.println("remove(0,10)");
    Serial.println(string);
  }

  uppercaseString = string;
  uppercaseString.toUpperCase();
// If LNURL, decode it in uppercase to Url
  if (uppercaseString.startsWith("LNURL")) {
    Serial.println(uppercaseString);
    Serial.println("Lets decode it ..\n");
    string = decode(uppercaseString);
  }

  // make sure it has a protocol, otherwise use https:// 
  UriComponents uriComponents = UriComponents::Parse(string.c_str());
  String host = uriComponents.host.c_str();
  String protocol = uriComponents.protocol.c_str();
  if (protocol == "") {
    protocol = "https://";
    string = protocol + string;
  }
 
  return string;
}

String decode(String lnUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  down = false;

  if(!lnUrl.startsWith("LNURL")) {
    log_e("found LNURL");
    return lnUrl;
  }

  if(!client.connect(lnbitsServer.c_str(), 443)) {
    Serial.println("Client couldn't connect to LNBitsServer to decode LNURL");
    down = true;
    return "";   
  }

  log_e();
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
  
  log_e();
  return doc["domain"];
}
