#include <assert.h>
#include <iostream>
#include "mock_libs/Arduino.h"
#include "../hamming.h"
#include "../crypto.h"

void test_hamming() {
    std::cout << "Testing Hamming(7,4)..." << std::endl;
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t encoded = encodeNibble(i);
        uint8_t decoded = decodeNibble(encoded);
        assert(decoded == i);

        // Test single bit error correction
        for (int bit = 0; bit < 7; bit++) {
            uint8_t corrupted = encoded ^ (1 << bit);
            uint8_t corrected = decodeNibble(corrupted);
            assert(corrected == i);
        }
    }
    std::cout << "Hamming(7,4) tests passed." << std::endl;
}

void test_crypto() {
    std::cout << "Testing Crypto functions..." << std::endl;

    // Test GF multiplication
    assert(gf_mul(0x02, 0x03) == 0x06);

    // Test polynomial expansion
    uint8_t coeffs[2] = {1, 2}; // f(x) = 1 + 2x
    uint8_t output[3];
    expand_poly(coeffs, 2, output, 3);
    assert(output[0] == 1); // f(0) = 1
    assert(output[1] == (1 ^ gf_mul(2, 1))); // f(1) = 1 + 2 = 3
    assert(output[2] == (1 ^ gf_mul(2, 2))); // f(2) = 1 + 4 = 5

    // Test XOR cipher
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t key[4] = {0x11, 0x22, 0x33, 0x44};
    xor_cipher(data, 4, key);
    assert(data[0] == (0xAA ^ 0x11));
    xor_cipher(data, 4, key);
    assert(data[0] == 0xAA);

    std::cout << "Crypto tests passed." << std::endl;
}

void test_crc8() {
    std::cout << "Testing CRC8..." << std::endl;
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t crc1 = crc8(data1, 3);

    uint8_t data2[] = {0x01, 0x02, 0x03};
    uint8_t crc2 = crc8(data2, 3);
    assert(crc1 == crc2);

    uint8_t data3[] = {0x01, 0x02, 0x04};
    uint8_t crc3 = crc8(data3, 3);
    assert(crc1 != crc3);

    std::cout << "CRC8 tests passed." << std::endl;
}

int main() {
    test_hamming();
    test_crypto();
    test_crc8();
    std::cout << "All unit tests passed!" << std::endl;
    return 0;
}
