#include <assert.h>
#include <iostream>
#include <deque>
#include "mock_libs/Arduino.h"
#include "mock_libs/IRremote/IRremote.h"
#include "mock_libs/WiFi/WiFi.h"
#include "mock_libs/SD/SD.h"
#include "mock_libs/EEPROM/EEPROM.h"

#include "../hamming.h"
#include "../crypto.h"

extern std::deque<uint16_t> irQueue;
extern EEPROMMock EEPROM_TX;
extern EEPROMMock EEPROM_RX;

char last_ssid[33] = {0};
char last_pass[65] = {0};

void connectToWiFi_mock(const char* ssid, const char* pass) {
    strncpy(last_ssid, ssid, 32);
    strncpy(last_pass, pass, 64);
    std::cout << "MOCK: Connecting to WiFi: " << ssid << " / " << pass << std::endl;
}

// Global defines to avoid multiple inclusion of headers in the .ino files
#define HAMMING_H
#define CRYPTO_H

namespace Transmitter {
#define EEPROM EEPROM_TX
#include "../ir_wifi_blaster_sd.ino"
#undef EEPROM
}

namespace Receiver {
#define EEPROM EEPROM_RX
#define connectToWiFi(s, p) connectToWiFi_mock(s, p)
#define void_connectToWiFi_body_dummy connectToWiFi_original
#include "../ir_wifi_receiver_sd.ino"
#undef connectToWiFi
#undef EEPROM
}

void init_sd_cards() {
    SDHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "IRWIFI01", 8);
    header.totalBlocks = 100;
    for (int i = 0; i < 32; i++) header.masterKey[i] = i;

    Transmitter::card.writeBlock(0, (uint8_t*)&header);
    Receiver::card.writeBlock(0, (uint8_t*)&header);

    uint8_t keyBlock[512];
    for (int i = 0; i < 512; i++) keyBlock[i] = (i + 0x55) % 256;
    Transmitter::card.writeBlock(1, keyBlock);
    Receiver::card.writeBlock(1, keyBlock);
}

int main() {
    std::cout << "Testing SD IR Blaster and Receiver..." << std::endl;

    init_sd_cards();

    // Initialize EEPROM
    uint32_t one = 1;
    EEPROM_TX.put(0, one);
    EEPROM_RX.put(0, one);
    uint32_t zero = 0;
    EEPROM_TX.put(4, zero);
    EEPROM_RX.put(4, zero);

    // Initialize
    std::cout << "INIT Transmitter" << std::endl;
    Transmitter::setup();

    std::cout << "INIT Receiver" << std::endl;
    Receiver::setup();

    // Check if they are actually the same address
    std::cout << "Address of Transmitter::keyBlock: " << (void*)Transmitter::keyBlock << std::endl;
    std::cout << "Address of Receiver::keyBlock: " << (void*)Receiver::keyBlock << std::endl;

    // Run transmitter loop once to send credentials
    std::cout << "LOOP Transmitter" << std::endl;
    Transmitter::loop();

    std::cout << "IR Queue size: " << irQueue.size() << std::endl;

    // Run receiver loop until all IR data is processed
    int max_iterations = 2000;
    while (max_iterations-- > 0 && !irQueue.empty()) {
        Receiver::loop();
    }

    // Verify
    std::cout << "Resulting SSID: " << last_ssid << std::endl;
    std::cout << "Resulting Pass: " << last_pass << std::endl;
    assert(strcmp(last_ssid, "YourSSID") == 0);
    assert(strcmp(last_pass, "YourPassword") == 0);

    std::cout << "SD IR integration test passed!" << std::endl;
    return 0;
}
