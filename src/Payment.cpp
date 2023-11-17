#include "Payment.h"
#include "Display.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "UriComponents.h"

#if SHOW_MY_PAYMENT_DEBUG_SERIAL
#define MY_PAYMENT_DEBUG_SERIAL Serial
#endif

Payment::Payment()
    : _amount("0"), _lnbitsServer(""), _invoiceKey(""), _dataId(""), _description(""), _payReq("") {};

void Payment::configure(const char*  amount, const char*  lnbitsServer, const char*  invoiceKey) {
  _amount = amount;
  _lnbitsServer = lnbitsServer;
  _invoiceKey = invoiceKey;
};

int Payment::getVendingPrice() {
  return _amount.toInt();
}

bool Payment::payWithLnUrlWithdrawl(String url) {
  String lnUrl = getUrl(url);
  if (lnUrl == "") { return false; }

#if !DEMO
  Withdrawal withdrawal = getWithdrawal(lnUrl);

  if(withdrawal.tag != "withdrawRequest") {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(F("Scanned tag is not LNURL withdraw"));
    MY_PAYMENT_DEBUG_SERIAL.println(F("Present a tag with a LNURL withdraw on it"));
    #endif
    displayErrorScreen(F("Withdrawal Failure"),F("Scanned tag is not LNURL withdraw \r\nPresent a tag with a LNURL withdraw on it"));
    return false;
  }

  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(F("Scanned tag is a LNURL withdrawal request!"));
  #endif
  if(!isAmountInWithdrawableBounds(_amount.toInt(), withdrawal.minWithdrawable,  withdrawal.maxWithdrawable)) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("The requested amount: " + _amount + " is not within this LNURL withdrawal bounds");
    MY_PAYMENT_DEBUG_SERIAL.println(F("Amount not in bounds, can't withdraw from presented voucher."));
    #endif
    displayErrorScreen(F("Withdrawal Failure"), "Amount not in bounds.\n Card only alows amounts between: " + String(withdrawal.minWithdrawable) + " - " + String(withdrawal.maxWithdrawable) + " sats");
    return false;
  }

  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println("The requested amount: " + _amount);
  MY_PAYMENT_DEBUG_SERIAL.println(F(" is within this LNURL withdrawal bounds"));
  MY_PAYMENT_DEBUG_SERIAL.println(F("Continue payment flow by creating invoice"));

  MY_PAYMENT_DEBUG_SERIAL.println(F("Will create invoice to request withrawal..."));
  #endif

#endif
  

  displayScreen("", "Creating invoice..");

#if !DEMO
  Invoice invoice = getInvoice("BitcoinSwitch QR");
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println("invoice.paymentHash = " + invoice.paymentHash); 
  MY_PAYMENT_DEBUG_SERIAL.println("invoice.paymentRequest = " + invoice.paymentRequest);
  MY_PAYMENT_DEBUG_SERIAL.println("invoice.checkingId = " + invoice.checkingId);  
  MY_PAYMENT_DEBUG_SERIAL.println("invoice.lnurlResponse = " + invoice.lnurlResponse); 
  MY_PAYMENT_DEBUG_SERIAL.println(F(""));
  #endif

  if(invoice.paymentRequest == "") {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(F("Failed to create invoice"));
    #endif
    displayErrorScreen(F("Withdrawal Failure"), F("Failed to create invoice")); 
    return false;
  }
#endif

#if DEMO
  delay(300);
#endif

  displayScreen("", F("Requesting withdrawal..")); 

#if !DEMO
  bool success = withdraw(withdrawal.callback, withdrawal.k1, invoice.paymentRequest);
  if(!success) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(F("Failed to request withdrawalfor invoice request with memo: ")); // + invoice.memo);
    #endif
    displayErrorScreen(F("Withdrawal Failure"), ("Failed to request withdrawal invoice"));
    return false;
  } 

  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(F("Withdrawal request successfull!"));
  #endif
  //TODO: Check if open invoice is paid
  bool isPaid = checkInvoice(invoice.checkingId);

  int numberOfTries = 1;
#endif

#if DEMO
  delay(500);
#endif
  displayScreen("", F("Waiting for payment confirmation..")); 
  
#if !DEMO
  while(!isPaid && (numberOfTries < 3)) {
    delay(2000);
    isPaid = checkInvoice(invoice.checkingId);
    numberOfTries++;
  }

  if(!isPaid) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(F("Could not confirm withdrawal, the invoice has not been payed in time"));
    #endif
    displayErrorScreen(F("Withdrawal Failure"), F("Could not confirm withdrawal, transaction cancelled"));
    return false;
  }
#endif

#if DEMO
  delay(800);
#endif

  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(F("Withdrawal successfull, invoice is payed!"));
  #endif
  displayScreen("", F("Withdrawal succeeded!! Thank you!"));
  return true;
}

Withdrawal Payment::getWithdrawal(String uri) {
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println("uri: " + uri);
  #endif
  WiFiClientSecure client;
  client.setInsecure();
  _down = false;
  UriComponents uriComponents = UriComponents::Parse(uri.c_str());
  String host = uriComponents.host.c_str();

  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println("host: " + host);
  #endif

  if(!client.connect(host.c_str(), 443)) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("Client couldn't connect to service: " + host + " to get Withdrawl");
    #endif
    _down = true;
    // return {};   
  }
  
  String request = String("GET ") + uri + " HTTP/1.0\r\n" +
    "User-Agent: ESP32\r\n" +
    "accept: text/html\r\n" +
    "Host: " + host + "\r\n\r\n";
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(request);
  #endif
  client.print(request);

  while(client.connected()) {
   String line = client.readStringUntil('\n');
   if(line == "\r") {
     break;
    }
  }
  String line = client.readString();
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(line);
  #endif
  
  const size_t capacity = JSON_OBJECT_SIZE(2) + 800;
  DynamicJsonDocument doc(capacity);  
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(("deserializeJson() failed: ") + String(error.f_str()));
    #endif
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

bool Payment::isAmountInWithdrawableBounds(int amount, int minWithdrawable, int maxWithdrawable) {
  int amountInMilliSats = amount * 1000;
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println("((" + String(amountInMilliSats) + " >= " + String(minWithdrawable) + ") && (" + String(amountInMilliSats) + " <= " + String(maxWithdrawable) + "))");
  #endif
  return ((amountInMilliSats >=minWithdrawable) && (amountInMilliSats <= maxWithdrawable));
}

Invoice Payment::getInvoice(String description) 
{
  WiFiClientSecure client;
  client.setInsecure();
  _down = false;

  if(!client.connect(_lnbitsServer.c_str(), 443)) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("Client couldn't connect to LNBitsServer to create Invoice");
    #endif
    _down = true;
    return {};   
  }

  String topost = "{\"out\": false,\"amount\" : " + String(_amount) + ", \"memo\" :\""+ String(description) + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  String request = (String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + _lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ _invoiceKey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");
  client.print(request);
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(request);
  #endif
  
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
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("deserializeJson() failed: " + String(error.f_str()));
    #endif
    return {};
  }

  //TODO: remove these user struct instsead
  const char* payment_hash = doc["checking_id"];
  const char* payment_request = doc["payment_request"];
  _payReq = payment_request;
  _dataId = payment_hash;

  return {
    doc["payment_hash"],
    doc["payment_request"],
    doc["checking_id"],
    doc["lnurl_response"]
  };
}

bool Payment::withdraw(String callback, String k1, String pr) {
  WiFiClientSecure client;
  client.setInsecure();
  UriComponents uriComponents = UriComponents::Parse(callback.c_str());
  String host = uriComponents.host.c_str();
  _down = false;

  if(!client.connect(host.c_str(), 443)) {
    _down = true;
    return {};   
  }
  String requestParameters = ("k1=" + k1 + "&pr=" + pr);
  String request = (String("GET ") + callback + "?" + requestParameters + " HTTP/1.0\r\n" +
                "User-Agent: ESP32\r\n" +
                "accept: text/plain\r\n" +
                "Host: " + host + "\r\n\r\n");
  client.print(request);
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(request);
  #endif
  
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(line);
  #endif
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("deserializeJson() failed: " + String(error.f_str()));
    #endif
    return false;
  }
  bool isOk = doc["status"];
  return isOk;
}

bool Payment::checkInvoice(String invoiceId) {
  WiFiClientSecure client;
  client.setInsecure();
  _down = false;

  if(!client.connect(_lnbitsServer.c_str(), 443)) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("Client couldn't connect to LNBitsServer to check Invoice");
    #endif
    _down = true;
    return false;   
  }

  String url = "/api/v1/payments/";
  String request = (String("GET ") + url + invoiceId +" HTTP/1.1\r\n" +
                "Host: " + _lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
  client.print(request);
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(request);
  #endif
  
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(line);
  #endif
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("deserializeJson() failed: " + String(error.f_str()));
    #endif
    return false;
  }
  bool isPaid = doc["paid"];
  return isPaid;
}

String Payment::getUrl(String string) {
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(string);
  #endif

  //Find the index of "://"
  int index = string.indexOf("://");
  
  // If "://" is found remove the uri from the string
  if (index != -1) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(string.substring(0,index));
    #endif
    string.remove(0,index + 3);
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(string);
    #endif
  }

  // If the string starts with "Lightning:"" remove it
  String uppercaseString = string;
  uppercaseString.toUpperCase();
  if (uppercaseString.startsWith("LIGHTNING:")) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL  
    MY_PAYMENT_DEBUG_SERIAL.println("string.startsWith(LIGHTNING:");
    MY_PAYMENT_DEBUG_SERIAL.println(string);
    #endif
    string.remove(0,10);
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("remove(0,10)");
    MY_PAYMENT_DEBUG_SERIAL.println(string);
    #endif
  }

  uppercaseString = string;
  uppercaseString.toUpperCase();
// If LNURL, decode it in uppercase to Url
  if (uppercaseString.startsWith("LNURL")) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println(uppercaseString);
    MY_PAYMENT_DEBUG_SERIAL.println("Lets decode it ..\n");
    #endif
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

String Payment::decode(String lnUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  _down = false;

  if(!lnUrl.startsWith("LNURL")) {
    log_e("found LNURL");
    return lnUrl;
  }

  if(!client.connect(_lnbitsServer.c_str(), 443)) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("Client couldn't connect to LNBitsServer to decode LNURL");
    #endif
    _down = true;
    return "";   
  }

  log_e();
  String body = "{\"data\": \"" + lnUrl + "\"}";
  String url = "/api/v1/payments/decode";
  String request = String("POST ") + url + " HTTP/1.1\r\n" +
                "Host: " + _lnbitsServer + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + body.length() + "\r\n" +
                "\r\n" + 
                body + "\n";
  client.print(request);
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(request);
  #endif
   
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if(line == "\r") {
      break;
    }
  }
  String line = client.readString();
  #ifdef MY_PAYMENT_DEBUG_SERIAL
  MY_PAYMENT_DEBUG_SERIAL.println(line);
  #endif
  const size_t capacity = JSON_OBJECT_SIZE(2) + 800;
  DynamicJsonDocument doc(capacity);  
  
  DeserializationError error = deserializeJson(doc, line);
  if(error) {
    #ifdef MY_PAYMENT_DEBUG_SERIAL
    MY_PAYMENT_DEBUG_SERIAL.println("deserializeJson() failed: " + String(error.f_str()));
    #endif
    return "";
  }
  
  log_e();
  return doc["domain"];
}
