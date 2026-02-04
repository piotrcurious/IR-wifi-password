#include <IRremote.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "hamming.h"
#include "crypto.h"

#if defined(ESP8266) || defined(ESP32)
#define EEPROM_SIZE 16
#endif

// Pin configuration
const int IR_SEND_PIN = 3;
const int SD_CS_PIN = 4;

// Protocol constants
const uint16_t PREAMBLE_CODE = 0x3FFF;
const int PREAMBLE_COUNT = 3;
const int SEND_DELAY = 45;
const int POLY_COEFFS_COUNT = 8;
const int EEPROM_ADDR_BLOCK = 0;
const int EEPROM_ADDR_ROLLING = 4;

Sd2Card card;
SDHeader header;
uint32_t currentBlock = 1; // Start from block 1, block 0 is header
uint32_t rollingCode = 0;
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
        Serial.println("SD init failed!");
        while(1);
    }

    // Read header from Block 0
    if (!card.readBlock(0, (uint8_t*)&header)) {
        Serial.println("Failed to read header!");
        while(1);
    }

    if (strncmp(header.magic, "IRWIFI01", 8) != 0) {
        Serial.println("Invalid SD header magic!");
        while(1);
    }

    EEPROM.get(EEPROM_ADDR_BLOCK, currentBlock);
    if (currentBlock == 0xFFFFFFFF || currentBlock == 0) currentBlock = 1;

    EEPROM.get(EEPROM_ADDR_ROLLING, rollingCode);
    if (rollingCode == 0xFFFFFFFF) rollingCode = 0;

    Serial.print("SD Blaster Ready. Total Blocks: ");
    Serial.print(header.totalBlocks);
    Serial.print(" Current Block: ");
    Serial.println(currentBlock);
}

void loop() {
    if (currentBlock >= header.totalBlocks) {
        Serial.println("Reached end of codebook!");
        delay(10000);
        return;
    }

    if (!card.readBlock(currentBlock, keyBlock)) {
        Serial.println("Read failed!");
        delay(5000);
        return;
    }

    rollingCode++;
    EEPROM.put(EEPROM_ADDR_ROLLING, rollingCode);
#if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
#endif

    // 1. Prepare and encrypt SyncInfo
    SyncInfo sync;
    sync.blockIndex = currentBlock;
    sync.rollingCode = rollingCode;
    sync.crc = crc8((uint8_t*)&sync, 8);

    uint8_t encryptedSync[sizeof(SyncInfo)];
    memcpy(encryptedSync, &sync, sizeof(SyncInfo));
    xor_cipher(encryptedSync, sizeof(SyncInfo), header.masterKey);

    // 2. Prepare Data Packet
    uint8_t ssidLen = strlen(ssid);
    uint8_t passLen = strlen(password);
    dataBuffer[0] = ssidLen;
    dataBuffer[1] = passLen;
    memcpy(dataBuffer + 2, ssid, ssidLen);
    memcpy(dataBuffer + 2 + ssidLen, password, passLen);

    uint8_t newCoeffs[POLY_COEFFS_COUNT];
    for (int i = 0; i < POLY_COEFFS_COUNT; i++) newCoeffs[i] = (uint8_t)random(256);
    memcpy(dataBuffer + 2 + ssidLen + passLen, newCoeffs, POLY_COEFFS_COUNT);

    uint8_t totalDataLen = 2 + ssidLen + passLen + POLY_COEFFS_COUNT;
    uint8_t crc = crc8(dataBuffer, totalDataLen);
    dataBuffer[totalDataLen] = crc;
    totalDataLen++;

    // Encrypt data with the key block
    xor_cipher(dataBuffer, totalDataLen, keyBlock);

    Serial.println("Blasting...");
    for (int i = 0; i < PREAMBLE_COUNT; i++) sendRC5Word(PREAMBLE_CODE);

    // Send encrypted SyncInfo
    for (size_t i = 0; i < sizeof(SyncInfo); i++) sendByte(encryptedSync[i]);

    // Send encrypted Data
    for (int i = 0; i < totalDataLen; i++) sendByte(dataBuffer[i]);

    // Key Evolution: update current block with new expansion
    expand_poly(newCoeffs, POLY_COEFFS_COUNT, keyBlock, 512);
    if (card.writeBlock(currentBlock, keyBlock)) {
        currentBlock++;
        EEPROM.put(EEPROM_ADDR_BLOCK, currentBlock);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
    }

    Serial.println("Waiting 10s...");
    delay(10000);
}
