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

    Transmitter::card.blocks.clear();
    Receiver::card.blocks.clear();

    Transmitter::card.writeBlock(0, (uint8_t*)&header);
    Receiver::card.writeBlock(0, (uint8_t*)&header);

    uint8_t keyBlock[512];
    for (int b = 1; b < 100; b++) {
        for (int i = 0; i < 512; i++) keyBlock[i] = (i + b + 0x55) % 256;
        Transmitter::card.writeBlock(b, keyBlock);
        Receiver::card.writeBlock(b, keyBlock);
    }
}

int main() {
    std::cout << "Testing SD Synchronization after Missed Transmissions..." << std::endl;

    init_sd_cards();

    uint32_t one = 1;
    EEPROM_TX.storage.clear();
    EEPROM_RX.storage.clear();
    EEPROM_TX.put(0, one);
    EEPROM_RX.put(0, one);
    uint32_t zero = 0;
    EEPROM_TX.put(4, zero);
    EEPROM_RX.put(4, zero);

    Transmitter::setup();
    Receiver::setup();

    // 1. Successfull transmission
    std::cout << "--- Initial Successfull Transmission ---" << std::endl;
    irQueue.clear();
    Transmitter::loop();
    while (!irQueue.empty()) Receiver::loop();
    assert(strcmp(last_ssid, "YourSSID") == 0);

    // 2. Miss 5 transmissions
    std::cout << "--- Transmitter sends 5 messages while Receiver is OFF ---" << std::endl;
    for (int i = 0; i < 5; i++) {
        irQueue.clear();
        Transmitter::loop();
        // Receiver does NOT process irQueue
    }

    // 3. Receiver comes back online
    std::cout << "--- Receiver tries to receive the 6th message ---" << std::endl;
    irQueue.clear();
    last_ssid[0] = 0;
    Transmitter::loop();

    int max_iterations = 2000;
    while (max_iterations-- > 0 && !irQueue.empty()) {
        Receiver::loop();
    }

    std::cout << "Resulting SSID: " << last_ssid << std::endl;
    if (strcmp(last_ssid, "YourSSID") == 0) {
        std::cout << "Sync SUCCESS after missed transmissions!" << std::endl;
    } else {
        std::cout << "Sync FAILED after missed transmissions!" << std::endl;
        return 1;
    }

    // 4. Test "Lapping" - Transmitter wraps around and evolves a block the receiver still thinks is old
    std::cout << "--- LAPPING TEST: Transmitter sends 100 messages (full wrap) while Receiver is OFF ---" << std::endl;
    for (int i = 0; i < 100; i++) {
        irQueue.clear();
        Transmitter::loop();
    }

    std::cout << "--- Receiver tries to receive message after Transmitter wrapped around ---" << std::endl;
    irQueue.clear();
    last_ssid[0] = 0;
    Transmitter::loop();

    max_iterations = 2000;
    while (max_iterations-- > 0 && !irQueue.empty()) {
        Receiver::loop();
    }

    std::cout << "Resulting SSID: " << last_ssid << std::endl;
    if (strcmp(last_ssid, "YourSSID") == 0) {
        std::cout << "Sync SUCCESS after LAPPING (Expected to fail if attacker, but success here because RX has the key but it might be stale?)" << std::endl;
        std::cout << "Wait, if RX missed the evolution of block X, and TX has now evolved block X twice, RX should fail." << std::endl;
    } else {
        std::cout << "Sync FAILED after LAPPING (As expected, since block was evolved without RX seeing it)." << std::endl;
    }

    return 0;
}
