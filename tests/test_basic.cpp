#include <assert.h>
#include <iostream>
#include <deque>
#include "mock_libs/Arduino.h"
#include "mock_libs/IRremote/IRremote.h"
#include "mock_libs/WiFi/WiFi.h"

#include "../hamming.h"

extern std::deque<uint16_t> irQueue;

char last_ssid[33] = {0};
char last_pass[65] = {0};

void connectToWiFi_mock(const char* ssid, const char* pass) {
    strncpy(last_ssid, ssid, 32);
    strncpy(last_pass, pass, 64);
    std::cout << "MOCK: Connecting to WiFi: " << ssid << " / " << pass << std::endl;
}

#define HAMMING_H

// Redefine setup and loop for transmitter and receiver
namespace Transmitter {
#define setup transmitter_setup
#define loop transmitter_loop
#include "../ir_wifi_blaster.ino"
#undef setup
#undef loop
}

namespace Receiver {
#define setup receiver_setup
#define loop receiver_loop
#define connectToWiFi(s, p) connectToWiFi_mock(s, p)
#define void_connectToWiFi_body_dummy connectToWiFi_original
#include "../ir_wifi_receiver.ino"
#undef setup
#undef loop
#undef connectToWiFi
}

int main() {
    std::cout << "Testing Basic IR Blaster and Receiver..." << std::endl;

    // Initialize
    Transmitter::transmitter_setup();
    Receiver::receiver_setup();

    // Run transmitter loop once to send credentials
    Transmitter::transmitter_loop();

    std::cout << "IR Queue size: " << irQueue.size() << std::endl;

    // Run receiver loop until all IR data is processed
    int max_iterations = 1000;
    while (max_iterations-- > 0 && !irQueue.empty()) {
        Receiver::receiver_loop();
    }

    // Verify
    std::cout << "Resulting SSID: " << last_ssid << std::endl;
    std::cout << "Resulting Pass: " << last_pass << std::endl;
    assert(strcmp(last_ssid, "YourSSID") == 0);
    assert(strcmp(last_pass, "YourPassword") == 0);

    std::cout << "Basic IR integration test passed!" << std::endl;
    return 0;
}
