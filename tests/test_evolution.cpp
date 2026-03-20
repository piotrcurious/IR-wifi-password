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
    header.totalBlocks = 10; // Smaller number for testing wrap around
    for (int i = 0; i < 32; i++) header.masterKey[i] = i;

    Transmitter::card.blocks.clear();
    Receiver::card.blocks.clear();

    Transmitter::card.writeBlock(0, (uint8_t*)&header);
    Receiver::card.writeBlock(0, (uint8_t*)&header);

    uint8_t keyBlock[512];
    for (int b = 1; b < 10; b++) {
        for (int i = 0; i < 512; i++) keyBlock[i] = (i + b + 0x55) % 256;
        Transmitter::card.writeBlock(b, keyBlock);
        Receiver::card.writeBlock(b, keyBlock);
    }
}

int main() {
    std::cout << "Testing SD Key Evolution..." << std::endl;

    init_sd_cards();

    // Initialize EEPROM
    uint32_t one = 1;
    EEPROM_TX.storage.clear();
    EEPROM_RX.storage.clear();
    EEPROM_TX.put(0, one);
    EEPROM_RX.put(0, one);
    uint32_t zero = 0;
    EEPROM_TX.put(4, zero);
    EEPROM_RX.put(4, zero);

    // Initialize
    Transmitter::setup();
    Receiver::setup();

    for (int trans = 0; trans < 20; trans++) {
        std::cout << "--- Transmission " << trans + 1 << " ---" << std::endl;
        irQueue.clear();
        last_ssid[0] = 0;
        last_pass[0] = 0;

        Transmitter::loop();

        int max_iterations = 2000;
        while (max_iterations-- > 0 && !irQueue.empty()) {
            Receiver::loop();
        }

        assert(strcmp(last_ssid, "YourSSID") == 0);
        assert(strcmp(last_pass, "YourPassword") == 0);

        // Verify both are on the same block
        uint32_t txBlock, rxBlock;
        EEPROM_TX.get(0, txBlock);
        EEPROM_RX.get(0, rxBlock);
        std::cout << "End of trans " << trans + 1 << ": TX Block=" << txBlock << ", RX Block=" << rxBlock << std::endl;
        assert(txBlock == rxBlock);

        // Verify key evolution (they should have written the same new block)
        uint8_t txKey[512], rxKey[512];
        uint32_t prevBlock = (txBlock == 1) ? 9 : txBlock - 1;
        Transmitter::card.readBlock(prevBlock, txKey);
        Receiver::card.readBlock(prevBlock, rxKey);
        assert(memcmp(txKey, rxKey, 512) == 0);
    }

    std::cout << "SD Key Evolution tests passed!" << std::endl;
    return 0;
}
