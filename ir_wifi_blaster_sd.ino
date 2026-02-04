#include <IRremote.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "hamming.h"
#include "crypto.h"

#if defined(ESP8266) || defined(ESP32)
#define EEPROM_SIZE 4
#endif

// Pin configuration
const int IR_SEND_PIN = 3;
const int SD_CS_PIN = 4;

// Protocol constants
const uint16_t PREAMBLE_CODE = 0x3FFF;
const int PREAMBLE_COUNT = 3;
const int SEND_DELAY = 50;
const int POLY_COEFFS_COUNT = 8;
const int EEPROM_ADDR = 0;

Sd2Card card;
uint32_t currentBlock = 0;
uint8_t keyBlock[512];
uint8_t dataBuffer[128];

const char* ssid = "YourSSID";
const char* password = "YourPassword";

void sendRC5Word(uint16_t word) {
    IrSender.sendRC5(word, 14);
    delay(SEND_DELAY);
}

void sendByte(uint8_t b) {
    uint8_t high = (b >> 4) & 0x0F;
    uint8_t low = b & 0x0F;
    sendRC5Word(0x2000 | encodeNibble(high));
    sendRC5Word(0x2000 | encodeNibble(low));
}

void setup() {
    Serial.begin(115200);
    IrSender.begin(IR_SEND_PIN);

#if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(EEPROM_SIZE);
#endif

    if (!card.init(SPI_HALF_SPEED, SD_CS_PIN)) {
        Serial.println("SD init failed! Please check SD card and CS pin.");
        while(1);
    }

    EEPROM.get(EEPROM_ADDR, currentBlock);
    if (currentBlock == 0xFFFFFFFF) currentBlock = 0;

    Serial.print("IR WiFi SD Blaster Ready. Current Block: ");
    Serial.println(currentBlock);
}

void loop() {
    Serial.println("Reading key block from SD...");
    if (!card.readBlock(currentBlock, keyBlock)) {
        Serial.println("Read failed!");
        delay(5000);
        return;
    }

    uint8_t ssidLen = strlen(ssid);
    uint8_t passLen = strlen(password);

    // Prepare data packet
    dataBuffer[0] = ssidLen;
    dataBuffer[1] = passLen;
    memcpy(dataBuffer + 2, ssid, ssidLen);
    memcpy(dataBuffer + 2 + ssidLen, password, passLen);

    // Generate random new poly coefficients for key regeneration
    uint8_t newCoeffs[POLY_COEFFS_COUNT];
    for (int i = 0; i < POLY_COEFFS_COUNT; i++) {
        newCoeffs[i] = (uint8_t)random(256);
    }
    memcpy(dataBuffer + 2 + ssidLen + passLen, newCoeffs, POLY_COEFFS_COUNT);

    // Calculate CRC of the data (before encryption)
    uint8_t totalDataLen = 2 + ssidLen + passLen + POLY_COEFFS_COUNT;
    uint8_t crc = crc8(dataBuffer, totalDataLen);
    dataBuffer[totalDataLen] = crc;
    totalDataLen++;

    // Encrypt everything with the key block
    xor_cipher(dataBuffer, totalDataLen, keyBlock);

    Serial.println("Blasting encrypted credentials...");
    for (int i = 0; i < PREAMBLE_COUNT; i++) {
        sendRC5Word(PREAMBLE_CODE);
    }

    for (int i = 0; i < totalDataLen; i++) {
        sendByte(dataBuffer[i]);
    }

    // Regenerate key block for the current index and save back to SD
    // This implements the "poly expansion" to extend the life of the system.
    Serial.println("Updating SD block with new expanded key...");
    expand_poly(newCoeffs, POLY_COEFFS_COUNT, keyBlock, 512);
    if (!card.writeBlock(currentBlock, keyBlock)) {
        Serial.println("Write failed!");
    } else {
        // Move to next block for next transmission
        currentBlock++;
        EEPROM.put(EEPROM_ADDR, currentBlock);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        Serial.print("Success. Next block: ");
        Serial.println(currentBlock);
    }

    Serial.println("Waiting 10 seconds...");
    delay(10000);
}
