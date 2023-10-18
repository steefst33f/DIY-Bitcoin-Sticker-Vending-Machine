#pragma once

#ifndef Dispenser_h
#define Dispenser_h

#include <Arduino.h>
#include "Display.h"
#include <ESP32Servo.h>

Servo servo;

//GPIO
int vendorModePin = 33;
int servoPin = 27;
int fillDispencerButton = 0;
int emptyDispencerButton = 35;

/// Servo Code
//////// Servo ///////

void begin() {
    //setup dispenser
    servo.attach(servoPin);
    pinMode(fillDispencerButton, INPUT);
    pinMode(emptyDispencerButton, INPUT);
}

void dispense() {
  Serial.println(F("Vending Machine dispense START!"));
  servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Vending Machine dispense STOP!!"));
}

void fillDispenser() {
  Serial.println(F("Fill dispenser!!"));
  // let dispencer slowly turn in the fillup direction, so the vender can empty the dispendser with new products:
  servo.writeMicroseconds(2000); // rotate counter clockwise (from the buyers point of view)
  delay(1590);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Done!"));
}

void emptyDispenser() {
  Serial.println(F("Empty dispenser!!"));
  // let dispencer slowly turn in dispence direction, so the vender can empty all products from dispenser:
  servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  servo.writeMicroseconds(1500);  // stop
  Serial.println(F("Done!"));
}

void vendorMode() {
int timer = 0;
  while(timer < 2000) {
    //Setup vendor mode
    Serial.println("vendorPin: " + String(touchRead(vendorModePin)));
    if(touchRead(vendorModePin) < 50) {
      Serial.println(F("In Vendor Fill/Empty mode"));
      Serial.println(F("(Restart Vending Machine to exit)"));
      displayVendorMode();

      ////Dispenser buttons scanning////
      while (true) {
        if (digitalRead(fillDispencerButton) == LOW) {
          fillDispenser();
        } else if(digitalRead(emptyDispencerButton) == LOW) {
          emptyDispenser();
        }
        delay(500);
      }  
    }
    timer = timer + 100;
    delay(300);
  }
}

#endif