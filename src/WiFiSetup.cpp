#include "WiFiSetup.h"
#include "SPIFFS.h"

WiFiSetup::WiFiSetup(const char* portalSsid, const char* portalPassword)
    : _portalSsid(portalSsid), _portalPassword(portalPassword), _portalIp(192, 168, 4, 1), _server(80), _dnsServer() {}

void WiFiSetup::begin() {
    String savedSsid = getConfiguredSsid();
    String savedPassword = getConfiguredPassword();

    if (!connectToWifi(savedSsid, savedPassword)) {
        startConfigurationPortal();
    } 
}

bool WiFiSetup::isWifiStatusConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiSetup::connectToWifi(String ssid, String password) {
    int trials = 0;
    int maxTrials = 3;
    WiFi.begin(ssid.c_str(), password.c_str());
    while ((WiFi.status() != WL_CONNECTED)) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        trials++;
        if (trials > maxTrials) {
        Serial.println("Failed to connect to WiFi");
        return false;
        }
    }

    Serial.println("Connected to WiFi");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(getLocalIp());
    return true;
}

void WiFiSetup::startConfigurationPortal() {
    // Create ESP32 AP
    WiFi.softAP(_portalSsid, _portalPassword);
    WiFi.softAPConfig(_portalIp, _portalIp, IPAddress(255, 255, 255, 0));
    _dnsServer.start(53, "*", _portalIp);

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP().toString());
    Serial.print("AP SSID: ");
    Serial.println(_portalSsid);
    Serial.print("AP password: ");
    Serial.println(_portalPassword);

    // Define web server routes
    _server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getConfigurationPage());
    });

    _server.on("/configure", HTTP_POST, [&](AsyncWebServerRequest *request) {
        String ssid = request->arg("ssid");
        String password = request->arg("password");
        saveConfiguration(ssid, password);

        if (connectToWifi(ssid, password)) {
            request->send(200, "text/plain", "Configuration successful!");
        } else {
            request->send(400, "text/plain", "Invalid configuration!");
        }
    });

    _server.onNotFound([](AsyncWebServerRequest* request){
        Serial.println("**client gets redeirected to: /login ***");
        request->redirect("http:" + WiFi.softAPIP().toString());
    });

    // Start the server
    _server.begin();
}

String WiFiSetup::getConfigurationPage() {
    String savedSsid = loadConfiguration("ssid");
    String savedPassword = loadConfiguration("password");

    String html = R"(
        <html>
        <body>
            <h1>Wi-Fi Configuration</h1>
            <form action='/configure' method='post'>
                <label for='ssid'>SSID (Network Name):</label>
                <input type='text' id='ssid' name='ssid' placeholder='Saved SSID' value=')" + savedSsid + R"('>
                <br><br>
                <label for='password'>Password:</label>
                <input type='password' id='password' name='password' placeholder='Saved Password' value=')" + savedPassword + R"('>
                <br><br>
                <input type='submit' value='Save Configuration'>
            </form>
        </body>
        </html>
    )";

    return html;
}

String WiFiSetup::loadConfiguration(const char* key) {
    _preferences.begin("config", true);
    String value = _preferences.getString(key, "");
    _preferences.end();
    return value;
}

void WiFiSetup::saveConfiguration(const String& ssid, const String& password) {
    _preferences.begin("config", false);
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
    _preferences.end();
}

wifi_event_id_t WiFiSetup::handleWifiEvents(WiFiEventCb eventHandler, arduino_event_id_t eventId) {
    //TODO: Create handlers for each event separatly to suppress spamming log messages?
    return WiFi.onEvent(eventHandler);
}

String WiFiSetup::getConfiguredSsid() {
    return loadConfiguration("ssid");
}

String WiFiSetup::getConfiguredPassword() {
    return loadConfiguration("password");
}

String WiFiSetup::getLocalIp() {
    return WiFi.localIP().toString();
}

String WiFiSetup::getPortalSsid() {
    return _portalSsid;
}

String WiFiSetup::getPortalPassword() {
    return _portalPassword;
}

String WiFiSetup::getPortalIp() {
    return _portalIp.toString();
}

void WiFiSetup::processDnsServerRequests() {
    _dnsServer.processNextRequest();
}
