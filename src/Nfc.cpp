#include "Nfc.h"

#if NFC_SPI
#define PN532_SCK  (25)
#define PN532_MISO (27)
#define PN532_MOSI (26)
#define PN532_SS   (33)

//TODO: use differernt pins, make sure they can all be used as output pins (except MISO as input)!
// SPIClass * vspi = NULL;
#endif

Nfc::Nfc(Adafruit_PN532 *nfcModule): nfcAdapter(nfcModule) {
    state = State::notConnected;
}

bool Nfc::isNfcModuleAvailable() {
    return _hasFoundNfcModule;
}

// Actions ////////////////////////////////

void Nfc::begin() {
    resetModule();
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

void Nfc::resetModule() {
  digitalWrite(36, LOW);
  delay(300);
  digitalWrite(36, HIGH);
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
    Serial.println("readTagMessage");
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
        Serial.println("recordCount: " + recordCount);
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
            
            Serial.println("Payload Length = " + String(payloadLength));
            Serial.println("  Information (as String): " + stringRecord);

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
    Serial.println("Connected to NFC Module!");
    _hasFoundNfcModule = true;
    state = State::idle;
    _onNfcModuleConnected();
}

void Nfc::connectionNfcModuleFailed() {
    Serial.println("Failed to connect to NFC Module!");
    _hasFoundNfcModule = false;
    state = State::notConnected;
    _onFailure(Error::connectionModuleFailed);
}

void Nfc::startScanningForTag() {
     state = State::scanning;
    _onStartScanningTag();
}

void Nfc::tagFound() {
    Serial.println("Tag Found!");
    if (state == State::scanning) {
        state = State::inlisting;
        identifyTag();
    }
}

void Nfc::noTagFound() {
    Serial.println("No Tag Found!");
}

void Nfc::tagIdentifiedSuccess(/*ISO14443aTag tag*/) {
    Serial.println("Tag Identified!");
    if (state == State::inlisting) {
        state = State::reading;
        _onReadingTag(/*tag*/);
        readTagMessage();
    }
}

void Nfc::tagIdentifyFailed() {
    Serial.println("Tag NOT Identified!");
    if (state == State::inlisting) {
        state = State::releasing;
        releaseTag();
        _onFailure(Error::identifyFailed);
    }
}

void Nfc::readSuccess(String stringRecord) {
    Serial.println("Read success!");
    if (state == State::reading) {
        state = State::releasing;
        releaseTag();
        _onReadMessageRecord(stringRecord);
    }
}

void Nfc::readFailed() {
    Serial.println("Read failed!");
    if (state == State::reading) {
        state = State::releasing;
        releaseTag();
        _onFailure(Error::readFailed);
    }
}

void Nfc::tagReleased() {
    Serial.println("Tag Released!");
    if (state == State::releasing) {
        state = State::idle;
    }
}