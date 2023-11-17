#include "WiFiConfiguration.h"
#include "SPIFFS.h"

#if SHOW_MY_WIFI_DEBUG_SERIAL
#define MY_WIFI_DEBUG_SERIAL Serial
#endif

WiFiConfiguration::WiFiConfiguration(const char* portalSsid, const char* portalPassword)
    : _portalSsid(portalSsid), _portalPassword(portalPassword), _portalIp(192, 168, 4, 1), _server(80), _dnsServer() {}

void WiFiConfiguration::begin() {
    String savedSsid = getConfiguredSsid();
    String savedPassword = getConfiguredPassword();

    if (!connectToWifi(savedSsid, savedPassword)) {
        startConfigurationPortal();
    } 
}

bool WiFiConfiguration::isWifiStatusConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiConfiguration::connectToWifi(String ssid, String password) {
    int trials = 0;
    int maxTrials = 3;
    WiFi.begin(ssid.c_str(), password.c_str());
    while ((WiFi.status() != WL_CONNECTED)) {
        delay(1000);
        #ifdef MY_WIFI_DEBUG_SERIAL
        MY_WIFI_DEBUG_SERIAL.println("Connecting to WiFi...");
        #endif
        trials++;
        if (trials > maxTrials) {
        #ifdef MY_WIFI_DEBUG_SERIAL
        MY_WIFI_DEBUG_SERIAL.println("Failed to connect to WiFi");
        #endif
        return false;
        }
    }
    #ifdef MY_WIFI_DEBUG_SERIAL
    MY_WIFI_DEBUG_SERIAL.println("Connected to WiFi");
    MY_WIFI_DEBUG_SERIAL.print("SSID: ");
    MY_WIFI_DEBUG_SERIAL.println(WiFi.SSID());
    MY_WIFI_DEBUG_SERIAL.print("IP address: ");
    MY_WIFI_DEBUG_SERIAL.println(getLocalIp());
    #endif
    return true;
}

void WiFiConfiguration::startConfigurationPortal() {
    // Create ESP32 AP
    WiFi.softAP(_portalSsid, _portalPassword);
    WiFi.softAPConfig(_portalIp, _portalIp, IPAddress(255, 255, 255, 0));
    _dnsServer.start(53, "*", _portalIp);

    #ifdef MY_WIFI_DEBUG_SERIAL
    MY_WIFI_DEBUG_SERIAL.print("AP IP address: ");
    MY_WIFI_DEBUG_SERIAL.println(WiFi.softAPIP().toString());
    MY_WIFI_DEBUG_SERIAL.print("AP SSID: ");
    MY_WIFI_DEBUG_SERIAL.println(_portalSsid);
    MY_WIFI_DEBUG_SERIAL.print("AP password: ");
    MY_WIFI_DEBUG_SERIAL.println(_portalPassword);
    #endif

    // Define web server routes
    _server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getConfigurationPage());
    });

    _server.on("/configure", HTTP_POST, [&](AsyncWebServerRequest *request) {
        String ssid = request->arg("ssid");
        String password = request->arg("password");
        String amount = request->arg("amount");
        String lnbitsServer = request->arg("lnbitsServer");
        String invoiceKey = request->arg("invoiceKey");
        saveConfiguration(ssid, password, amount, lnbitsServer, invoiceKey);

        if (connectToWifi(ssid, password)) {
            request->send(200, "text/plain", "Configuration successful!");
        } else {
            request->send(400, "text/plain", "Invalid configuration!");
        }
    });

    _server.onNotFound([](AsyncWebServerRequest* request){
        #ifdef MY_WIFI_DEBUG_SERIAL
        MY_WIFI_DEBUG_SERIAL.println("**client gets redeirected to: /login ***");
        #endif
        request->redirect("http:" + WiFi.softAPIP().toString());
    });

    // Start the server
    _server.begin();
    processDnsServerRequests();

}

String WiFiConfiguration::getConfigurationPage() {
    String configuredSsid = loadConfiguration("ssid");
    String configuredPassword = loadConfiguration("password");
    String configuredAmount = loadConfiguration("amount");
    String configuredLnbitsServer = loadConfiguration("lnbitsServer");
    String configuredInvoiceKey = loadConfiguration("invoiceKey");

    String html = R"(
        <html>
        <body>
            <h1> Vending Machine Configuration</h1>
            <form action='/configure' method='post'>
                <h2>Wi-Fi</h2>
                <label for='ssid'>SSID (Network Name):</label>
                <input type='text' id='ssid' name='ssid' placeholder='Configured SSID' value=')" + configuredSsid + R"('>
                <br><br>
                <label for='password'>Password:</label>
                <input type='password' id='password' name='password' placeholder='Configured Password' value=')" + configuredPassword + R"('>
                <br><br> 
                <br><br>     
                <br><br> 
                <h2>LNBits</h2>    
                <label for='amount'>Amount (to pay in Sats for Product):</label>
                <input type='text' id='amount' name='amount' placeholder='Configured Amount' value=')" + configuredAmount + R"('>
                <br><br>
                <label for='lnbitsServer'>lnbitsServer:</label>
                <input type='lnbitsServer' id='lnbitsServer' name='lnbitsServer' placeholder='Saved lnbitsServer' value=')" + configuredLnbitsServer + R"('>
                <br><br>
                <label for='invoiceKey'>invoiceKey:</label>
                <input type='invoiceKey' id='invoiceKey' name='invoiceKey' placeholder='Saved invoiceKey' value=')" + configuredInvoiceKey + R"('>
                <br><br>
                <input type='submit' value='Save Configuration'>
            </form>
        </body>
        </html>
    )";

    return html;
}

String WiFiConfiguration::loadConfiguration(const char* key) {
    _preferences.begin("config", true);
    String value = _preferences.getString(key, "");
    _preferences.end();
    return value;
}

void WiFiConfiguration::saveConfiguration(const String& ssid, const String& password, const String& amount, const String& lnbitsServer, const String& invoiceKey) {
    _preferences.begin("config", false);
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
    _preferences.putString("amount", amount);
    _preferences.putString("lnbitsServer", lnbitsServer);
    _preferences.putString("invoiceKey", invoiceKey);
    _preferences.end();
}

wifi_event_id_t WiFiConfiguration::handleWifiEvents(WiFiEventCb eventHandler, arduino_event_id_t eventId) {
    //TODO: Create handlers for each event separatly to suppress spamming log messages?
    return WiFi.onEvent(eventHandler);
}

String WiFiConfiguration::getConfiguredSsid() {
    return loadConfiguration("ssid");
}

String WiFiConfiguration::getConfiguredPassword() {
    return loadConfiguration("password");
}

String WiFiConfiguration::getConfiguredAmount() {
    return loadConfiguration("amount");
}

String WiFiConfiguration::getConfiguredLnbitsServer() {
    return loadConfiguration("lnbitsServer");
}

String WiFiConfiguration::getConfiguredInvoiceKey() {
    return loadConfiguration("invoiceKey");
}

String WiFiConfiguration::getLocalIp() {
    return WiFi.localIP().toString();
}

String WiFiConfiguration::getPortalSsid() {
    return _portalSsid;
}

String WiFiConfiguration::getPortalPassword() {
    return _portalPassword;
}

String WiFiConfiguration::getPortalIp() {
    return _portalIp.toString();
}

void WiFiConfiguration::processDnsServerRequests() {
    _dnsServer.processNextRequest();
}
