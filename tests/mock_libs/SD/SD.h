#ifndef SD_H
#define SD_H

#include <stdint.h>
#include <map>
#include <vector>

#define SPI_HALF_SPEED 0

class Sd2Card {
public:
    bool init(int speed, int pin);
    bool readBlock(uint32_t block, uint8_t* dst);
    bool writeBlock(uint32_t block, const uint8_t* src);

    std::map<uint32_t, std::vector<uint8_t>> blocks;
};

#endif
