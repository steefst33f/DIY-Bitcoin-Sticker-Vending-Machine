#include "dispenser.h"

#if SHOW_MY_DISPENSER_DEBUG_SERIAL
#define MY_DISPENSER_DEBUG_SERIAL Serial
#endif

Dispenser::Dispenser(uint8_t vendorModePin, uint8_t servoPin , uint8_t fillDispencerButton, uint8_t emptyDispencerButton) 
: _vendorModePin(vendorModePin), _servoPin(servoPin), _fillDispencerButton(fillDispencerButton), _emptyDispencerButton(emptyDispencerButton) {};

Dispenser::~Dispenser() {};

void Dispenser::begin() {
    //setup dispenser
    _servo.attach(_servoPin);
    pinMode(_fillDispencerButton, INPUT);
    pinMode(_emptyDispencerButton, INPUT);
};

void Dispenser::dispense() {
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Vending Machine dispense START!"));
  #endif
  _servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  _servo.writeMicroseconds(1500);  // stop
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Vending Machine dispense STOP!!"));
  #endif
}

void Dispenser::waitForVendorMode(int timeout /* = 2000*/) {
int timer = 0;
  while(timer < timeout) {
    #ifdef MY_DISPENSER_DEBUG_SERIAL
    MY_DISPENSER_DEBUG_SERIAL.println("vendorPin: " + String(touchRead(_vendorModePin)));
    #endif
    if(touchRead(_vendorModePin) < 50) {
      vendorMode();
    }
    timer = timer + 100;
    delay(300);
  }
}

void Dispenser::vendorMode() {
    #ifdef MY_DISPENSER_DEBUG_SERIAL
    MY_DISPENSER_DEBUG_SERIAL.println(F("In Vendor Fill/Empty mode"));
    MY_DISPENSER_DEBUG_SERIAL.println(F("(Restart Vending Machine to exit)"));
    #endif
    displayVendorMode();

    ////Dispenser buttons scanning////
    while (true) {
        if (digitalRead(_fillDispencerButton) == LOW) {
            fillDispenser();
        } else if(digitalRead(_emptyDispencerButton) == LOW) {
            emptyDispenser();
        }
        delay(500);
    }  
}

void Dispenser::fillDispenser() {
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Fill dispenser!!"));
  #endif
  // let dispencer slowly turn in the fillup direction, so the vender can empty the dispendser with new products:
  _servo.writeMicroseconds(2000); // rotate counter clockwise (from the buyers point of view)
  delay(1590);
  _servo.writeMicroseconds(1500);  // stop
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Done!"));
  #endif
}

void Dispenser::emptyDispenser() {
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Empty dispenser!!"));
  #endif
  // let dispencer slowly turn in dispence direction, so the vender can empty all products from dispenser:
  _servo.writeMicroseconds(1000); // rotate clockwise (from the buyers point of view)
  delay(1920);
  _servo.writeMicroseconds(1500);  // stop
  #ifdef MY_DISPENSER_DEBUG_SERIAL
  MY_DISPENSER_DEBUG_SERIAL.println(F("Done!"));
  #endif
}