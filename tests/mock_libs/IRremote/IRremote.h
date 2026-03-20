#ifndef IRREMOTE_H
#define IRREMOTE_H

#include <stdint.h>
#include <vector>

#define RC5 1
#define ENABLE_LED_FEEDBACK true

struct IRData {
    uint8_t protocol;
    uint32_t decodedRawData;
};

class IrSenderMock {
public:
    void begin(int pin) {}
    void sendRC5(uint16_t data, int bits);
};

class IrReceiverMock {
public:
    IRData decodedIRData;
    void begin(int pin, bool feedback) {}
    bool decode();
    void resume() {}
    void queue(uint16_t data);
private:
    std::vector<uint16_t> buffer;
};

extern IrSenderMock IrSender;
extern IrReceiverMock IrReceiver;

#endif
