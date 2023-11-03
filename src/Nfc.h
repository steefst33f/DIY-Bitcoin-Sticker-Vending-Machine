#include <Arduino.h>

#pragma once

#ifndef Nfc_h
#define Nfc_h

#include <Wire.h>
#include <PN532.h>

#if NFC_SPI
#include <PN532_SPI.h>
#else
#include <PN532_I2C.h>
#endif

#include <NfcAdapter.h>

class Nfc {
    public:

    enum class State {
        notConnected,
        idle,
        scanning,
        inlisting,
        reading,
        releasing
    };

    enum class Error {
        readFailed,
        identifyFailed,
        releaseFailed,
        scanFailed,
        connectionModuleFailed,
        unknownError
    };

    NfcAdapter nfcModule;

    Nfc();
    void begin();
    void powerDownMode();
    void scanForTag();
    void resetModule();

    // Actions
    void identifyTag();
    void readTagMessage();
    void releaseTag();

    // Handlers
    void setOnNfcModuleConnected(std::function<void(void)> setOnNfcModuleConnected);
    void setOnStartScanningTag(std::function<void(void)> onStartScanningTag);
    void setOnReadMessageRecord(std::function<void(String)> onReadMessageRecord);
    void setOnReadingTag(std::function<void(/*ISO14443aTag*/)> onReadingTag);
    void setOnFailure(std::function<void(Error)> onFailure);

    bool isNfcModuleAvailable();

    String convertToStringFromBytes(uint8_t dataArray[], int sizeOfArray) {
        String stringOfData = "";
        for(int byteIndex = 0; byteIndex < sizeOfArray; byteIndex++) {
            stringOfData += (char)dataArray[byteIndex];
        }
        return stringOfData;
    }

    private:
    #if NFC_SPI
    PN532_SPI _pn532_spi;
    #else
    PN532_I2C _pn532_i2c;
    #endif
    State state;
    bool _hasFoundNfcModule;

    //Events
    void connectedToNfcModule();
    void connectionNfcModuleFailed();
    void startScanningForTag();
    void tagFound();
    void noTagFound();
    void tagIdentifiedSuccess(/*ISO14443aTag tag*/);
    void tagIdentifyFailed();
    void readSuccess(String payloadString);
    void readFailed();
    void tagReleased();

    //Callbacks
    std::function<void(void)> _onNfcModuleConnected;
    std::function<void(void)> _onStartScanningTag;
    std::function<void(String)> _onReadMessageRecord;
    std::function<void(/*ISO14443aTag*/)> _onReadingTag;
    std::function<void(Error)> _onFailure;
};

#endif
