#include <IRremote.h>
#include <avr/pgmspace.h>

const int IR_SEND_PIN = 3;  // IR transmitter pin
const char* ssid = "YourSSID";
const char* password = "YourPassword";

IRsend irsend(IR_SEND_PIN);

// Hamming (7,4) Encoding Table (0-15)
const uint8_t hamming74_encode[16] = {
    0b0000000, 0b0001110, 0b0010101, 0b0011011,
    0b0100011, 0b0101101, 0b0110110, 0b0111000,
    0b1000111, 0b1001001, 0b1010010, 0b1011100,
    0b1100100, 0b1101010, 0b1110001, 0b1111111
};

// Function to encode a 4-bit nibble with Hamming (7,4) ECC
uint8_t encodeHamming74(uint8_t nibble) {
    if (nibble > 0x0F) return 0;  // Sanity check; should be in range 0-15
    return hamming74_encode[nibble];
}

// Function to split, encode, and send a character over RC5
void sendRC5EncodedChar(char c) {
    uint8_t charData = (uint8_t)c;            // Convert char to byte
    uint8_t highNibble = (charData >> 4) & 0x0F;  // High 4 bits
    uint8_t lowNibble = charData & 0x0F;          // Low 4 bits

    uint8_t encodedHigh = encodeHamming74(highNibble);  // Encode high nibble
    uint8_t encodedLow = encodeHamming74(lowNibble);    // Encode low nibble

    // Send each nibble separately as 14-bit RC5 commands
    uint16_t rc5CommandHigh = (1 << 13) | (encodedHigh & 0x7F);  // Form 14-bit command
    uint16_t rc5CommandLow = (1 << 13) | (encodedLow & 0x7F);

    // Send the encoded data as RC5
    irsend.sendRC5(rc5CommandHigh, 14);
    delay(45); // Wait for receiver processing

    irsend.sendRC5(rc5CommandLow, 14);
    delay(45); // Wait for receiver processing
}

void sendStringAsRC5(const char* text) {
    while (*text) {
        sendRC5EncodedChar(*text++);
    }
}

void setup() {
    Serial.begin(9600);
    irsend.begin();

    // Send SSID
    Serial.println("Sending SSID as RC5 encoded sequence:");
    sendStringAsRC5(ssid);

    // Send Password
    Serial.println("Sending Password as RC5 encoded sequence:");
    sendStringAsRC5(password);
}

void loop() {
    // Do nothing
}
