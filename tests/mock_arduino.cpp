#include "Arduino.h"
#include "IRremote.h"
#include "SD.h"
#include "EEPROM.h"
#include "WiFi.h"
#include <vector>
#include <deque>
#include <iostream>

SerialMock Serial;
IrSenderMock IrSender;
IrReceiverMock IrReceiver;
WiFiMock WiFi;

EEPROMMock EEPROM_TX;
EEPROMMock EEPROM_RX;

std::deque<uint16_t> irQueue;

void IrSenderMock::sendRC5(uint16_t data, int bits) {
    irQueue.push_back(data);
}

bool IrReceiverMock::decode() {
    if (irQueue.empty()) return false;
    decodedIRData.protocol = RC5;
    decodedIRData.decodedRawData = irQueue.front();
    irQueue.pop_front();
    return true;
}

void IrReceiverMock::queue(uint16_t data) {
    irQueue.push_back(data);
}

bool Sd2Card::init(int speed, int pin) { return true; }

bool Sd2Card::readBlock(uint32_t block, uint8_t* dst) {
    if (blocks.count(block)) {
        memcpy(dst, blocks[block].data(), 512);
        return true;
    }
    return false;
}

bool Sd2Card::writeBlock(uint32_t block, const uint8_t* src) {
    std::vector<uint8_t> b(512);
    memcpy(b.data(), src, 512);
    blocks[block] = b;
    return true;
}
