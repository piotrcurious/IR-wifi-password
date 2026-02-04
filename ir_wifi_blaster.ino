#include <IRremote.h>
#include "hamming.h"

// Pin configuration
const int IR_SEND_PIN = 3;

// Protocol constants
const uint16_t PREAMBLE_CODE = 0x3FFF;
const int PREAMBLE_COUNT = 3;
const int SEND_DELAY = 50; // ms between RC5 commands

const char* ssid = "YourSSID";
const char* password = "YourPassword";

void sendRC5Word(uint16_t word) {
    IrSender.sendRC5(word, 14);
    delay(SEND_DELAY);
}

void sendByte(uint8_t b) {
    uint8_t high = (b >> 4) & 0x0F;
    uint8_t low = b & 0x0F;

    uint8_t encodedHigh = encodeNibble(high);
    uint8_t encodedLow = encodeNibble(low);

    sendRC5Word(0x2000 | encodedHigh);
    sendRC5Word(0x2000 | encodedLow);
}

void setup() {
    Serial.begin(115200);
    IrSender.begin(IR_SEND_PIN);
    Serial.println("IR WiFi Blaster Ready");
}

void loop() {
    Serial.println("Blasting WiFi credentials...");

    // 1. Preamble
    for (int i = 0; i < PREAMBLE_COUNT; i++) {
        sendRC5Word(PREAMBLE_CODE);
    }

    // Prepare data for CRC
    uint8_t ssidLen = strlen(ssid);
    uint8_t passLen = strlen(password);
    uint8_t totalLen = 2 + ssidLen + passLen;
    uint8_t buffer[128]; // Stack allocation

    if (totalLen > 128) {
        Serial.println("Error: SSID/Password too long!");
        return;
    }

    buffer[0] = ssidLen;
    buffer[1] = passLen;
    memcpy(buffer + 2, ssid, ssidLen);
    memcpy(buffer + 2 + ssidLen, password, passLen);

    uint8_t crc = crc8(buffer, totalLen);

    // 2. Send SSID Length
    sendByte(ssidLen);

    // 3. Send Password Length
    sendByte(passLen);

    // 4. Send SSID
    for (int i = 0; i < ssidLen; i++) {
        sendByte(ssid[i]);
    }

    // 5. Send Password
    for (int i = 0; i < passLen; i++) {
        sendByte(password[i]);
    }

    // 6. Send CRC
    sendByte(crc);

    Serial.println("Blast complete. Waiting 10 seconds...");
    delay(10000);
}
