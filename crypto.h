#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

// GF(2^8) multiplication using the irreducible polynomial x^8 + x^4 + x^3 + x^2 + 1 (0x11D)
inline uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi_bit_set = a & 0x80;
        a <<= 1;
        if (hi_bit_set) a ^= 0x1D;
        b >>= 1;
    }
    return p;
}

/**
 * Expands a set of polynomial coefficients into a larger block.
 * output[i] = coeffs[0] + coeffs[1]*i + coeffs[2]*i^2 + ... + coeffs[n-1]*i^(n-1)
 */
inline void expand_poly(const uint8_t *coeffs, size_t num_coeffs, uint8_t *output, size_t output_len) {
    for (size_t i = 0; i < output_len; i++) {
        uint8_t val = 0;
        uint8_t x = (uint8_t)(i & 0xFF); // Wrap around 256
        uint8_t x_pow = 1;
        for (size_t j = 0; j < num_coeffs; j++) {
            val ^= gf_mul(coeffs[j], x_pow);
            x_pow = gf_mul(x_pow, x);
        }
        output[i] = val;
    }
}

/**
 * Symmetric XOR cipher.
 */
inline void xor_cipher(uint8_t *data, size_t data_len, const uint8_t *key) {
    for (size_t i = 0; i < data_len; i++) {
        data[i] ^= key[i];
    }
}

struct SDHeader {
    char magic[8];        // "IRWIFI01"
    uint32_t totalBlocks;
    uint8_t masterKey[32];
    uint8_t reserved[468];
};

struct __attribute__((packed)) SyncInfo {
    uint32_t blockIndex;
    uint32_t rollingCode;
    uint8_t crc;           // CRC8 of blockIndex and rollingCode
};

#endif
