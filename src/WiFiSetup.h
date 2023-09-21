#include <Arduino.h>

#pragma once

#ifndef WiFiSetup_h
#define WiFiSetup_h

    #include <Preferences.h>
    #include <WiFi.h>
    #include <DNSServer.h>
    #include <ESPAsyncWebServer.h>

    Preferences preferences;
    DNSServer dnsServer;
    AsyncWebServer server(80);

    const String apSSID = "ESP32_AP";
    const String apPassword = "password123";
    IPAddress apIP(192, 168, 4, 1);

    String savedSSID = "";
    String savedPassword = "";

    void getSavedWifiCredentials();
    void setSavedWifiCredentials(String ssid, String password);
    bool connectToWifi(String ssid, String password);
    void startApWifiSetup(std::function<void(String, String)> onSuccess, std::function<void(String, String, String)> onFailure);
    void processDnsServerRequests();
    bool isWifiStatusConnected();
    wifi_event_id_t handleWifiEvents(WiFiEventCb cbEvent, arduino_event_id_t event = ARDUINO_EVENT_MAX);
    void wifiSetupSuccess(void (*onSuccess)(String, String), String ssid, String localIp);
    void wifiSetupFailed(void (*onFailure)(String, String, String), String apSsid, String apPassword, String apIp);

    // void (*onSuccess)(String, String);
    std::function<void(String, String)> _onSuccess;
    std::function<void(String, String, String)> _onFailure;

    void wifiSetupSuccess(void (*onSuccess)(String, String), String ssid, String localIp) {
        onSuccess(ssid, localIp);
    }

    void wifiSetupFailed(void (*onFailure)(String, String, String), String apSsid, String apPassword, String apIp) {
        onFailure(apSsid, apPassword, apIp);
    }

    String localIp() {
        return WiFi.localIP().toString();
    }

    wifi_event_id_t handleWifiEvents(WiFiEventCb eventHandler, arduino_event_id_t eventId) {
        //TODO: Create handlers for each event separatly to suppress spamming log messages?
        return WiFi.onEvent(eventHandler);
    }

    bool isWifiStatusConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    void getSavedWifiCredentials() {
        preferences.begin("wifi-config", false);
        savedSSID = preferences.getString("ssid", "");
        savedPassword = preferences.getString("password", "");
        preferences.end();
    }

    void setSavedWifiCredentials(String ssid, String password) {
        preferences.begin("wifi-config", false);
        savedSSID = preferences.putString("ssid", ssid);
        savedPassword = preferences.putString("password", password);
        preferences.end();
    }

    bool connectToWifi(String ssid, String password) {
        // Connect to Wi-Fi
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
        Serial.println(localIp());
        return true;
    }

    void startApWifiSetup(std::function<void(String, String)> onSuccess, std::function<void(String, String, String)> onFailure) {
    _onSuccess = onSuccess;
    _onFailure = onFailure;

    // Create ESP32 AP
    WiFi.softAP(apSSID, apPassword);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    dnsServer.start(53, "*", apIP);

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP().toString());
    Serial.print("AP SSID: ");
    Serial.println(apSSID);
    Serial.print("AP password: ");
    Serial.println(apPassword);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        getSavedWifiCredentials();
        // Serve the HTML page with saved credentials filled in
        String html = "<html><body>";
        html += "<h1>Wi-Fi Configuration</h1>";
        html += "<form action='/configure' method='post'>";
        html += "<label for='ssid'>SSID (Network Name):</label>";
        html += "<input type='text' id='ssid' name='ssid' placeholder='Saved SSID' value='" + savedSSID + "'>";
        html += "<br><br>";
        html += "<label for='password'>Password:</label>";
        html += "<input type='password' id='password' name='password' placeholder='Saved Password' value='" + savedPassword + "'>";
        html += "<br><br>";
        html += "<input type='submit' value='Save Configuration'>";
        html += "</form>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    server.onNotFound([](AsyncWebServerRequest* request){
            Serial.println("**client gets redeirected to: /login ***");
            request->redirect("http://" + apIP.toString());
        });

    server.on("/configure", HTTP_POST, [](AsyncWebServerRequest *request){
        String ssid = request->arg("ssid");
        String password = request->arg("password");

        setSavedWifiCredentials(ssid, password);
        getSavedWifiCredentials();

        if (connectToWifi(savedSSID, savedPassword)) {
            request->send(200, "text/plain", "Configuration successful!");
            _onSuccess(ssid, localIp());
        // displayWifiConnected(ssid, localIp());
        } else {
            request->send(400, "text/plain", "Invalid configuration!");
            _onFailure(apSSID, apPassword, apIP.toString());
        // displayWifiSetup(apSSID, apPassword, apIP.toString());
        }
    });

    server.begin();
}

void processDnsServerRequests() {
    dnsServer.processNextRequest();
}
#endif