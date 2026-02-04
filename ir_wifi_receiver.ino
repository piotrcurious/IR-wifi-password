#include <IRremote.hpp> // Use .hpp for version 4.x
#if defined(ESP8266)
const int IR_RECV_PIN = 14; // D5
#elif defined(ESP32)
const int IR_RECV_PIN = 15;
#else
const int IR_RECV_PIN = 2;
#endif

#include "hamming.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
// Non-WiFi device? Just print the credentials.
#endif

enum State {
    WAIT_PREAMBLE,
    RECEIVE_SSID_LEN,
    RECEIVE_PASS_LEN,
    RECEIVE_DATA,
    RECEIVE_CRC
};

State currentState = WAIT_PREAMBLE;
uint8_t ssidLen = 0;
uint8_t passLen = 0;
uint8_t buffer[128];
int bufferIndex = 0;
int nibbleIndex = 0;
uint8_t currentByte = 0;
int preambleSeen = 0;

void resetReceiver() {
    currentState = WAIT_PREAMBLE;
    bufferIndex = 0;
    nibbleIndex = 0;
    preambleSeen = 0;
    Serial.println("Receiver reset, waiting for preamble...");
}

void processNibble(uint8_t encoded) {
    uint8_t decoded = decodeNibble(encoded);
    if (nibbleIndex == 0) {
        currentByte = (decoded << 4);
        nibbleIndex = 1;
    } else {
        currentByte |= (decoded & 0x0F);
        buffer[bufferIndex++] = currentByte;
        nibbleIndex = 0;

        // State transitions based on bytes received
        if (currentState == RECEIVE_SSID_LEN) {
            ssidLen = currentByte;
            currentState = RECEIVE_PASS_LEN;
            Serial.print("SSID Len: "); Serial.println(ssidLen);
        } else if (currentState == RECEIVE_PASS_LEN) {
            passLen = currentByte;
            currentState = RECEIVE_DATA;
            Serial.print("Pass Len: "); Serial.println(passLen);
            if (ssidLen + passLen > 120) { // Safety check
                resetReceiver();
            }
        } else if (currentState == RECEIVE_DATA) {
            if (bufferIndex == (2 + ssidLen + passLen)) {
                currentState = RECEIVE_CRC;
            }
        } else if (currentState == RECEIVE_CRC) {
            uint8_t receivedCrc = currentByte;
            uint8_t computedCrc = crc8(buffer, 2 + ssidLen + passLen);

            if (receivedCrc == computedCrc) {
                Serial.println("CRC Match! Credentials received.");
                char ssid[33];
                char pass[65];
                memcpy(ssid, buffer + 2, ssidLen);
                ssid[ssidLen] = '\0';
                memcpy(pass, buffer + 2 + ssidLen, passLen);
                pass[passLen] = '\0';

                Serial.print("SSID: "); Serial.println(ssid);
                Serial.print("Pass: "); Serial.println(pass);

                connectToWiFi(ssid, pass);
            } else {
                Serial.print("CRC Mismatch! Calculated: "); Serial.print(computedCrc, HEX);
                Serial.print(" Received: "); Serial.println(receivedCrc, HEX);
            }
            resetReceiver();
        }
    }
}

void connectToWiFi(const char* ssid, const char* pass) {
#if defined(ESP8266) || defined(ESP32)
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Connection failed.");
    }
#else
    Serial.println("WiFi not supported on this board.");
#endif
}

void setup() {
    Serial.begin(115200);
#if defined(ESP8266) || defined(ESP32)
    WiFi.mode(WIFI_STA);
#endif
    IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);
    Serial.println("IR WiFi Receiver Ready");
    resetReceiver();
}

void loop() {
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol == RC5) {
            uint16_t value = IrReceiver.decodedIRData.decodedRawData;
            Serial.print("Received RC5: "); Serial.println(value, HEX);

            if (value == 0x3FFF) {
                preambleSeen++;
                if (preambleSeen >= 2) {
                    Serial.println("Preamble detected!");
                    currentState = RECEIVE_SSID_LEN;
                    bufferIndex = 0;
                    nibbleIndex = 0;
                }
            } else if (currentState != WAIT_PREAMBLE) {
                // Should have bit 13 set
                if (value & 0x2000) {
                    processNibble(value & 0x7F);
                }
            }
        }
        IrReceiver.resume();
    }
}
