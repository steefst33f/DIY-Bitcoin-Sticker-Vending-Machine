#include <Arduino.h>

#pragma once

#ifndef WiFiConfiguration_h
#define WiFiConfiguration_h

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <DNSServer.h>

class WiFiConfiguration {
public:
    WiFiConfiguration(const char* portalSsid, const char* portalPassword);
    void begin();

    void startConfigurationPortal();
    bool isWifiStatusConnected();
    void processDnsServerRequests();
    bool connectToWifi(String ssid, String password);

    String getConfiguredSsid();
    String getConfiguredPassword();
    String getLocalIp();
    String getConfiguredAmount();
    String getConfiguredLnbitsServer();
    String getConfiguredInvoiceKey();

    String getPortalSsid();
    String getPortalPassword();
    String getPortalIp();

    wifi_event_id_t handleWifiEvents(WiFiEventCb cbEvent, arduino_event_id_t event = ARDUINO_EVENT_MAX);

private:
    const char* _portalSsid;
    const char* _portalPassword;
    IPAddress _portalIp;

    AsyncWebServer _server;
    Preferences _preferences;
    DNSServer _dnsServer;

    String getConfigurationPage();
    String loadConfiguration(const char* key);
    void saveConfiguration(const String& ssid, const String& password, const String& amount, const String& lnbitsServer, const String& invoiceKey);
};
#endif