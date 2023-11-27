#pragma once

#ifndef AppConfiguration_h
#define AppConfiguration_h

#include <Arduino.h>
#include <improv.h>
#include <Esp.h>
#include <WiFi.h>

#define LED_BUILTIN 2
//*** Improv
#define MAX_ATTEMPTS_WIFI_CONNECTION 20

/*
 * C++ Singleton
 * Limitation: Single Threaded Design
 * See: http://www.aristeia.com/Papers/DDJ_Jul_Aug_2004_revised.pdf
 *      For problems associated with locking in multi threaded applications
 *
 * Limitation:
 * If you use this Singleton (A) within a destructor of another Singleton (B)
 * This Singleton (A) must be fully constructed before the constructor of (B)
 * is called.
 */
class AppConfiguration
{
public:
    // Function to get the singleton instance
    static AppConfiguration& getInstance() {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static AppConfiguration instance;
        return instance;
    }

    void begin();
    void end();
    void loop();

private:
    uint8_t _x_buffer[16];
    uint8_t _x_position;
    
    // Client variables 
    char _linebuf[80];
    int _charcount;

    //*** Web Server
    WiFiServer _server;

    AppConfiguration();
    ~AppConfiguration();
    AppConfiguration(AppConfiguration const& copy);            // Not Implemented
    AppConfiguration& operator=(AppConfiguration const& copy); // Not Implemented

    static void set_state(improv::State state);
    static void send_response(std::vector<uint8_t> &response);
    static void set_error(improv::Error error);

    static void onErrorCallback(improv::Error err);
    static bool onCommandCallback(improv::ImprovCommand cmd);
    static void getAvailableWifiNetworks();
    static std::vector<std::string> getLocalUrl();

    void handle_request();
    bool connectWifi(std::string ssid, std::string password);
    void blink_led(int d, int times);

};

#endif