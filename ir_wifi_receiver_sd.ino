#include <IRremote.hpp>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "hamming.h"
#include "crypto.h"

#if defined(ESP8266) || defined(ESP32)
#define EEPROM_SIZE 4
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
const int IR_RECV_PIN = 14;
const int SD_CS_PIN = 15;
#elif defined(ESP32)
#include <WiFi.h>
const int IR_RECV_PIN = 15;
const int SD_CS_PIN = 5;
#else
const int IR_RECV_PIN = 2;
const int SD_CS_PIN = 4;
#endif

const int POLY_COEFFS_COUNT = 8;
const int EEPROM_ADDR = 0;

Sd2Card card;
uint32_t currentBlock = 0;
uint8_t keyBlock[512];
uint8_t buffer[128];
int bufferIndex = 0;
int nibbleIndex = 0;
uint8_t currentByte = 0;
int preambleSeen = 0;
uint8_t ssidLen = 0;
uint8_t passLen = 0;

enum State {
    WAIT_PREAMBLE,
    RECEIVE_DATA
};

State currentState = WAIT_PREAMBLE;

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
    uint8_t receivedCrc = buffer[totalDataLen];
    uint8_t computedCrc = crc8(buffer, totalDataLen);

    if (receivedCrc == computedCrc) {
        Serial.println("CRC Match! Decryption successful.");

        char ssid[33];
        char pass[65];
        memcpy(ssid, buffer + 2, ssidLen);
        ssid[ssidLen] = '\0';
        memcpy(pass, buffer + 2 + ssidLen, passLen);
        pass[passLen] = '\0';

        uint8_t newCoeffs[POLY_COEFFS_COUNT];
        memcpy(newCoeffs, buffer + 2 + ssidLen + passLen, POLY_COEFFS_COUNT);

        Serial.print("SSID: "); Serial.println(ssid);

        // Update key block
        Serial.println("Updating SD block with new expansion...");
        expand_poly(newCoeffs, POLY_COEFFS_COUNT, keyBlock, 512);
        if (card.writeBlock(currentBlock, keyBlock)) {
            currentBlock++;
            EEPROM.put(EEPROM_ADDR, currentBlock);
#if defined(ESP8266) || defined(ESP32)
            EEPROM.commit();
#endif
            Serial.print("Block updated. Next block: "); Serial.println(currentBlock);
            // Read next block for next time
            if (!card.readBlock(currentBlock, keyBlock)) {
                Serial.println("Next block read failed!");
            }
        } else {
            Serial.println("SD Write Failed!");
        }

        connectToWiFi(ssid, pass);
    } else {
        Serial.println("CRC Mismatch! Decryption failed or out of sync.");
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

        // Decrypt byte using current key block
        buffer[bufferIndex] = currentByte ^ keyBlock[bufferIndex];

        if (bufferIndex == 0) {
            ssidLen = buffer[0];
            Serial.print("SSID Len (decrypted): "); Serial.println(ssidLen);
        } else if (bufferIndex == 1) {
            passLen = buffer[1];
            Serial.print("Pass Len (decrypted): "); Serial.println(passLen);
            if (ssidLen + passLen > 100) { // Safety
                resetReceiver();
                return;
            }
        }

        bufferIndex++;
        nibbleIndex = 0;

        uint8_t totalExpected = 2 + ssidLen + passLen + POLY_COEFFS_COUNT + 1;
        if (ssidLen > 0 && bufferIndex == totalExpected) {
            processDecryptedData();
        }
    }
}

void connectToWiFi(const char* ssid, const char* pass) {
#if defined(ESP8266) || defined(ESP32)
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected! IP: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("Connection failed.");
    }
#endif
}

void setup() {
    Serial.begin(115200);
    IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);

#if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(EEPROM_SIZE);
#endif

    if (!card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
        Serial.println("SD init failed!");
        while(1);
    }

    EEPROM.get(EEPROM_ADDR, currentBlock);
    if (currentBlock == 0xFFFFFFFF) currentBlock = 0;

    Serial.print("IR WiFi SD Receiver Ready. Block: "); Serial.println(currentBlock);

    // Pre-read the current key block
    if (!card.readBlock(currentBlock, keyBlock)) {
        Serial.println("SD Read Failed!");
    }

    resetReceiver();
}

void loop() {
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol == RC5) {
            uint16_t value = IrReceiver.decodedIRData.decodedRawData;
            if (value == 0x3FFF) {
                preambleSeen++;
                if (preambleSeen >= 2) {
                    Serial.println("Preamble detected!");
                    currentState = RECEIVE_DATA;
                    bufferIndex = 0;
                    nibbleIndex = 0;
                }
            } else if (currentState == RECEIVE_DATA) {
                if (value & 0x2000) {
                    processNibble(value & 0x7F);
                }
            }
        }
        IrReceiver.resume();
    }
}
