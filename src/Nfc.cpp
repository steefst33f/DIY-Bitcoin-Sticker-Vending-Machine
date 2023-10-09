#include "Nfc.h"

Nfc::Nfc(): _pn532_i2c(Wire), nfcModule(_pn532_i2c) {
    state = State::notConnected;
}

bool Nfc::isNfcModuleAvailable() {
    return _hasFoundNfcModule;
}

// Actions ////////////////////////////////

void Nfc::begin() {
    state = State::notConnected;
    if (nfcModule.begin()) {
        connectedToNfcModule();
    } else {
        connectionNfcModuleFailed();
    }
}

void Nfc::scanForTag() {
    state = State::scanning;
    startScanningForTag();
    nfcModule.powerDownMode();
    if (nfcModule.isTagPresent()) {
        tagFound();
    } else {
        noTagFound();
    }
}

void Nfc::identifyTag() {
    state = State::inlisting;
    if (nfcModule.identifyTag()) {
        tagIdentifiedSuccess(/*nfcModule.getInlistedTag()*/);
    } else {
        tagIdentifyFailed();
    }
}

void Nfc::readTagMessage() {
    state = State::reading;
    Serial.println("readTagMessage");
    NfcTag tag = nfcModule.read();
    if (tag.hasNdefMessage()) {
        log_e();
        NdefMessage message = tag.getNdefMessage();
        message.print();

        int recordCount = message.getRecordCount();
        Serial.println("recordCount: " + recordCount);
        if (recordCount < 1) {
            log_e();
            readFailed();
            return;
        }

        // If more than 1 Message then it wil cycle through them untill it finds a LNURL
        for(int i = 0; i < recordCount; i++) {
            NdefRecord record = message.getRecord(i);
            log_e();
            record.print();

            int payloadLength = record.getPayloadLength();
            if (payloadLength < 1) {
                log_e();
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
        log_e();
        readFailed();
        return;
    }
}

void Nfc::releaseTag() {
    state = State::releasing;
    nfcModule.releaseTag();
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
    Serial.println("Tag Identified!");
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