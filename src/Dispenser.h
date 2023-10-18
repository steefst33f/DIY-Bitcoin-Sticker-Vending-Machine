#pragma once

#ifndef Dispenser_h
#define Dispenser_h

#include <Arduino.h>
#include "Display.h"
#include <ESP32Servo.h>

/// Servo Code
//////// Servo ///////
class Dispenser
{

public:
    Dispenser(uint8_t vendorModePin, uint8_t servoPin , uint8_t fillDispencerButton, uint8_t emptyDispencerButton);
    ~Dispenser();
    void begin();
    void dispense();
    void waitForVendorMode(int timeout = 2000);

private:
    Servo _servo;
    int _vendorModePin;
    int _servoPin;
    int _fillDispencerButton;
    int _emptyDispencerButton;

    void fillDispenser();
    void emptyDispenser();
    void vendorMode();
};

#endif