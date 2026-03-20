#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))

#define HEX 16
#define DEC 10

class SerialMock {
public:
    void begin(unsigned long baud) {}
    void print(const char* s) { printf("%s", s); }
    void print(int n, int base = DEC) { if (base == HEX) printf("%X", (unsigned int)n); else printf("%d", n); }
    void print(unsigned int n, int base = DEC) { if (base == HEX) printf("%X", n); else printf("%u", n); }
    void print(long n, int base = DEC) { if (base == HEX) printf("%lX", (unsigned long)n); else printf("%ld", n); }
    void print(unsigned long n, int base = DEC) { if (base == HEX) printf("%lX", n); else printf("%lu", n); }
    void println(const char* s) { printf("%s\n", s); }
    void println(int n, int base = DEC) { if (base == HEX) printf("%X\n", (unsigned int)n); else printf("%d\n", n); }
    void println(unsigned int n, int base = DEC) { if (base == HEX) printf("%X\n", n); else printf("%u\n", n); }
    void println(long n, int base = DEC) { if (base == HEX) printf("%lX\n", (unsigned long)n); else printf("%ld\n", n); }
    void println(unsigned long n, int base = DEC) { if (base == HEX) printf("%lX\n", n); else printf("%lu\n", n); }
    void println() { printf("\n"); }
};

extern SerialMock Serial;

inline void delay(unsigned long ms) {}
inline long random(long max) { return rand() % max; }

#endif
