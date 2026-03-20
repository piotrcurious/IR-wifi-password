#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <map>
#include <iostream>

class EEPROMMock {
public:
    void begin(int size) {}
    void commit() {}

    template<typename T>
    void get(int addr, T& data) {
        uint8_t* ptr = (uint8_t*)&data;
        for (size_t i = 0; i < sizeof(T); i++) {
            if (storage.find(addr + i) != storage.end()) {
                ptr[i] = storage[addr + i];
            } else {
                ptr[i] = 0xFF;
            }
        }
    }

    template<typename T>
    void put(int addr, const T& data) {
        const uint8_t* ptr = (const uint8_t*)&data;
        for (size_t i = 0; i < sizeof(T); i++) {
            storage[addr + i] = ptr[i];
        }
    }

    std::map<int, uint8_t> storage;
};

extern EEPROMMock EEPROM_TX;
extern EEPROMMock EEPROM_RX;

#endif
