#include "Nfc.h"

#if NFC_SPI
#define PN532_SCK  (25)
#define PN532_MISO (27)
#define PN532_MOSI (26)
#define PN532_SS   (33)

//TODO: use differernt pins, make sure they can all be used as output pins (except MISO as input)!
// SPIClass * vspi = NULL;
#endif

#if SHOW_MY_NFC_DEBUG_SERIAL
#define MY_NFC_DEBUG_SERIAL Serial
#endif

Nfc::Nfc(Adafruit_PN532 *nfcModule): nfcAdapter(nfcModule) {
    state = State::notConnected;
}

bool Nfc::isNfcModuleAvailable() {
    return _hasFoundNfcModule;
}

// Actions ////////////////////////////////

void Nfc::begin() {
    state = State::notConnected;
    if (nfcAdapter.begin()) {
        connectedToNfcModule();
    } else {
        connectionNfcModuleFailed();
    }
}

void Nfc::powerDownMode() {
    // nfcAdapter.powerDownMode();
}

void Nfc::scanForTag() {
    state = State::scanning;
    startScanningForTag();
    powerDownMode();
    if (nfcAdapter.isTagPresent()) {
        tagFound();
    } else {
        noTagFound();
    }
}

void Nfc::identifyTag() {
    state = State::inlisting;
    if (nfcAdapter.identifyTag()) {
        tagIdentifiedSuccess(/*nfcAdapter.getInlistedTag()*/);
    } else {
        tagIdentifyFailed();
    }
}

void Nfc::readTagMessage() {
    state = State::reading;
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("readTagMessage");
    #endif
#if DEMO
    readSuccess("lightning:LNURL1DP68GURN8GHJ7MR9VAJKUEPWD3HXY6T5WVHXXMMD9AMKJARGV3EXZAE0V9CXJTMKXYHKCMN4WFKZ7KJTV3RX2JZ4FD28JDMGTP95U5Z3VDJNYAM294NPJY");
    return;
#endif
    NfcTag tag = nfcAdapter.read();
    if (tag.hasNdefMessage()) {
        // log_e();
        NdefMessage message = tag.getNdefMessage();
        message.print();

        int recordCount = message.getRecordCount();
        #ifdef MY_NFC_DEBUG_SERIAL
        MY_NFC_DEBUG_SERIAL.println("recordCount: " + recordCount);
        #endif
        if (recordCount < 1) {
            // log_e();
            readFailed();
            return;
        }

        // If more than 1 Message then it wil cycle through them untill it finds a LNURL
        for(int i = 0; i < recordCount; i++) {
            NdefRecord record = message.getRecord(i);
            // log_e();
            record.print();

            int payloadLength = record.getPayloadLength();
            if (payloadLength < 1) {
                // log_e();
                readFailed();
                return;
            }

            byte payload[record.getPayloadLength()];
            record.getPayload(payload);
            //+1 to skip first byte of payload, which is always null
            String stringRecord = convertToStringFromBytes(payload+1, payloadLength-1);
            
            #ifdef MY_NFC_DEBUG_SERIAL
            MY_NFC_DEBUG_SERIAL.println("Payload Length = " + String(payloadLength));
            MY_NFC_DEBUG_SERIAL.println("  Information (as String): " + stringRecord);
            #endif

            readSuccess(stringRecord);
            return;
        }
    } else {
        // log_e();
        readFailed();
        return;
    }
}

void Nfc::releaseTag() {
    state = State::releasing;
    nfcAdapter.releaseTag();
    tagReleased();
}


// Callbacks /////////////////////////////
void Nfc::setOnNfcModuleConnected(std::function<void(void)> onNfcModuleConnected) {
    _onNfcModuleConnected = onNfcModuleConnected;
}

void Nfc::setOnStartScanningTag(std::function<void(void)> onStartScanningTag) {
    _onStartScanningTag = onStartScanningTag;
}

void Nfc::setOnReadMessageRecord(std::function<void(String)> onReadMessageRecord) {
    _onReadMessageRecord = onReadMessageRecord;
}

void Nfc::setOnReadingTag(std::function<void(/*ISO14443aTag*/)> onReadingTag) {
    _onReadingTag = onReadingTag;
}

void Nfc::setOnFailure(std::function<void(Error)> onFailure) {
    _onFailure = onFailure;
}


// Events ////////////////////////////////

void Nfc::connectedToNfcModule() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Connected to NFC Module!");
    #endif
    _hasFoundNfcModule = true;
    state = State::idle;
    _onNfcModuleConnected();
}

void Nfc::connectionNfcModuleFailed() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Failed to connect to NFC Module!");
    #endif
    _hasFoundNfcModule = false;
    state = State::notConnected;
    _onFailure(Error::connectionModuleFailed);
}

void Nfc::startScanningForTag() {
     state = State::scanning;
    _onStartScanningTag();
}

void Nfc::tagFound() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Tag Found!");
    #endif
    if (state == State::scanning) {
        state = State::inlisting;
        identifyTag();
    }
}

void Nfc::noTagFound() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("No Tag Found!");
    #endif
}

void Nfc::tagIdentifiedSuccess(/*ISO14443aTag tag*/) {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Tag Identified!");
    #endif
    if (state == State::inlisting) {
        state = State::reading;
        _onReadingTag(/*tag*/);
        readTagMessage();
    }
}

void Nfc::tagIdentifyFailed() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Tag NOT Identified!");
    #endif
    if (state == State::inlisting) {
        state = State::releasing;
        releaseTag();
        _onFailure(Error::identifyFailed);
    }
}

void Nfc::readSuccess(String stringRecord) {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Read success!");
    #endif
    if (state == State::reading) {
        state = State::releasing;
        releaseTag();
        _onReadMessageRecord(stringRecord);
    }
}

void Nfc::readFailed() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Read failed!");
    #endif
    if (state == State::reading) {
        state = State::releasing;
        releaseTag();
        _onFailure(Error::readFailed);
    }
}

void Nfc::tagReleased() {
    #ifdef MY_NFC_DEBUG_SERIAL
    MY_NFC_DEBUG_SERIAL.println("Tag Released!");
    #endif
    if (state == State::releasing) {
        state = State::idle;
    }
}