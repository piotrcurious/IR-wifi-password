#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t RECV_PIN = 14;  // GPIO pin for IR receiver
const int bufferSize = 64;  // Max number of characters to receive (adjust as needed)

// Hamming (7,4) Decoding Table in PROGMEM
const uint8_t hamming74_decode[128] PROGMEM = {
    0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x02, 0x03, 0xFF, 
    0xFF, 0x04, 0x05, 0xFF, 0x06, 0xFF, 0xFF, 0x07,
    0xFF, 0x08, 0x09, 0xFF, 0x0A, 0xFF, 0xFF, 0x0B,
    0x0C, 0xFF, 0xFF, 0x0D, 0xFF, 0x0E, 0x0F, 0xFF,
    // ... fill in remaining entries to handle error cases
};

// Create IR receiver object
IRrecv irrecv(RECV_PIN);
decode_results results;

char receivedBuffer[bufferSize];
int bufferIndex = 0;

// Decode Hamming (7,4) data with error correction from PROGMEM table
uint8_t decodeHamming74(uint8_t data) {
    if (data >= 128) return 0xFF; // Out of range for Hamming (7,4)
    return pgm_read_byte(&hamming74_decode[data]);
}

// Process each 14-bit RC5 command to retrieve original nibbles
void processRC5Command(uint16_t rc5Command) {
    uint8_t encodedNibble = rc5Command & 0x7F; // Extract 7-bit encoded nibble
    uint8_t decodedNibble = decodeHamming74(encodedNibble);
    if (decodedNibble == 0xFF) {
        Serial.println("Error: Unable to decode nibble.");
        return; // Skip on uncorrectable error
    }
    receivedBuffer[bufferIndex++] = decodedNibble;  // Store decoded nibble
    if (bufferIndex >= bufferSize) bufferIndex = 0; // Wrap around if buffer full
}

void decodeAndPrintBuffer() {
    Serial.print("Decoded String: ");
    for (int i = 0; i < bufferIndex; i += 2) {
        if (i + 1 < bufferIndex) {
            char decodedChar = (receivedBuffer[i] << 4) | receivedBuffer[i + 1];
            Serial.print(decodedChar);
        }
    }
    Serial.println();
    bufferIndex = 0;  // Reset buffer after decoding
}

void setup() {
    Serial.begin(115200);
    irrecv.enableIRIn();  // Start the receiver
    Serial.println("Ready to receive IR signals...");
}

void loop() {
    if (irrecv.decode(&results)) {
        if (results.decode_type == RC5) {
            processRC5Command(results.value);  // Decode and store RC5 command
        }

        irrecv.resume();  // Receive the next value

        // Periodically decode the buffer if a certain number of characters have been received
        if (bufferIndex > 0 && bufferIndex % 2 == 0) {
            decodeAndPrintBuffer();
        }
    }
}
