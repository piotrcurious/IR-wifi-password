#include <IRremote.hpp>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "hamming.h"
#include "crypto.h"

#if defined(ESP8266) || defined(ESP32)
#define EEPROM_SIZE 16
#include <ESP8266WiFi.h>
#if defined(ESP8266)
const int IR_RECV_PIN = 14;
const int SD_CS_PIN = 15;
#else
#include <WiFi.h>
const int IR_RECV_PIN = 15;
const int SD_CS_PIN = 5;
#endif
#else
const int IR_RECV_PIN = 2;
const int SD_CS_PIN = 4;
#endif

const int POLY_COEFFS_COUNT = 8;
const int EEPROM_ADDR_BLOCK = 0;
const int EEPROM_ADDR_ROLLING = 4;

Sd2Card card;
SDHeader header;
uint32_t currentBlock = 1;
uint32_t lastRollingCode = 0;
uint8_t keyBlock[512];
uint8_t syncBuffer[sizeof(SyncInfo)];
uint8_t dataBuffer[128];

int bufferIndex = 0;
int nibbleIndex = 0;
uint8_t currentByte = 0;
int preambleSeen = 0;

enum State {
    WAIT_PREAMBLE,
    RECEIVE_SYNC,
    RECEIVE_DATA
};

State currentState = WAIT_PREAMBLE;
uint8_t ssidLen = 0, passLen = 0;

void resetReceiver() {
    currentState = WAIT_PREAMBLE;
    bufferIndex = 0;
    nibbleIndex = 0;
    preambleSeen = 0;
    ssidLen = 0;
    passLen = 0;
    Serial.println("Waiting for preamble...");
}

void processDecryptedData() {
    uint8_t totalDataLen = 2 + ssidLen + passLen + POLY_COEFFS_COUNT;
    uint8_t receivedCrc = dataBuffer[totalDataLen];
    uint8_t computedCrc = crc8(dataBuffer, totalDataLen);

    if (receivedCrc == computedCrc) {
        Serial.println("Data CRC Match!");
        char ssid[33], pass[65];
        memcpy(ssid, dataBuffer + 2, ssidLen); ssid[ssidLen] = '\0';
        memcpy(pass, dataBuffer + 2 + ssidLen, passLen); pass[passLen] = '\0';

        uint8_t newCoeffs[POLY_COEFFS_COUNT];
        memcpy(newCoeffs, dataBuffer + 2 + ssidLen + passLen, POLY_COEFFS_COUNT);

        expand_poly(newCoeffs, POLY_COEFFS_COUNT, keyBlock, 512);
        if (card.writeBlock(currentBlock, keyBlock)) {
            uint32_t nextBlock = currentBlock + 1;
            if (nextBlock >= header.totalBlocks) nextBlock = 1;
            EEPROM.put(EEPROM_ADDR_BLOCK, nextBlock);
            EEPROM.put(EEPROM_ADDR_ROLLING, lastRollingCode);
#if defined(ESP8266) || defined(ESP32)
            EEPROM.commit();
#endif
            Serial.println("SD & EEPROM updated.");
        }
        connectToWiFi(ssid, pass);
    } else {
        Serial.println("Data CRC Mismatch!");
    }
    resetReceiver();
}

void processNibble(uint8_t encoded) {
    uint8_t decoded = decodeNibble(encoded);
    if (nibbleIndex == 0) {
        currentByte = (decoded << 4);
        nibbleIndex = 1;
    } else {
        currentByte |= (decoded & 0x0F);
        nibbleIndex = 0;

        if (currentState == RECEIVE_SYNC) {
            syncBuffer[bufferIndex++] = currentByte;
            if (bufferIndex == sizeof(SyncInfo)) {
                // Decrypt SyncInfo
                xor_cipher(syncBuffer, sizeof(SyncInfo), header.masterKey);
                SyncInfo* sync = (SyncInfo*)syncBuffer;

                if (crc8(syncBuffer, 8) == sync->crc) {
                    if (sync->rollingCode > lastRollingCode) {
                        Serial.print("Sync OK. Block: "); Serial.println(sync->blockIndex);
                        currentBlock = sync->blockIndex;
                        lastRollingCode = sync->rollingCode;

                        if (card.readBlock(currentBlock, keyBlock)) {
                            currentState = RECEIVE_DATA;
                            bufferIndex = 0;
                        } else {
                            Serial.println("Block read failed!"); resetReceiver();
                        }
                    } else {
                        Serial.println("Old rolling code, ignoring (replay?)."); resetReceiver();
                    }
                } else {
                    Serial.println("Sync CRC fail!"); resetReceiver();
                }
            }
        } else if (currentState == RECEIVE_DATA) {
            dataBuffer[bufferIndex] = currentByte ^ keyBlock[bufferIndex];
            if (bufferIndex == 0) ssidLen = dataBuffer[0];
            else if (bufferIndex == 1) {
                passLen = dataBuffer[1];
                if (ssidLen + passLen > 100) { resetReceiver(); return; }
            }
            bufferIndex++;
            uint8_t totalExpected = 2 + ssidLen + passLen + POLY_COEFFS_COUNT + 1;
            if (ssidLen > 0 && bufferIndex == totalExpected) processDecryptedData();
        }
    }
}

void connectToWiFi(const char* ssid, const char* pass) {
#if defined(ESP8266) || defined(ESP32)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(500); Serial.print("."); attempts++; }
    if (WiFi.status() == WL_CONNECTED) { Serial.print("\nConnected! IP: "); Serial.println(WiFi.localIP()); }
#endif
}

void setup() {
    Serial.begin(115200);
    IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);
#if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(EEPROM_SIZE);
#endif
    if (!card.init(SPI_HALF_SPEED, SD_CS_PIN) || !card.readBlock(0, (uint8_t*)&header)) {
        Serial.println("SD/Header Error!"); while(1);
    }
    EEPROM.get(EEPROM_ADDR_BLOCK, currentBlock);
    if (currentBlock == 0xFFFFFFFF) currentBlock = 1;
    EEPROM.get(EEPROM_ADDR_ROLLING, lastRollingCode);
    if (lastRollingCode == 0xFFFFFFFF) lastRollingCode = 0;
    resetReceiver();
}

void loop() {
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol == RC5) {
            uint16_t val = IrReceiver.decodedIRData.decodedRawData;
            if (val == 0x3FFF) {
                preambleSeen++;
                if (preambleSeen >= 2) { currentState = RECEIVE_SYNC; bufferIndex = 0; nibbleIndex = 0; Serial.println("Preamble!"); }
            } else if (currentState != WAIT_PREAMBLE && (val & 0x2000)) {
                processNibble(val & 0x7F);
            }
        }
        IrReceiver.resume();
    }
}
