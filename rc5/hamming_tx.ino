#include <IRremote.h>

// Pin configuration
const int IR_LED_PIN = 3;

// RC5 protocol constants
const unsigned long RC5_BIT_TIME = 1778;  // Microseconds per bit in RC5
const unsigned int COMMAND_BITS = 14;     // Using full RC5 extended format
const unsigned int RETRY_COUNT = 3;       // Number of times to transmit each character
const unsigned long CHAR_DELAY = 100;     // Delay between characters in milliseconds

// Error correction using Hamming(7,4) code
const byte HAMMING_MATRIX[4] = {0b1111000, 0b1100110, 0b1010101, 0b0110011};

class WiFiIRTransmitter {
private:
    IRsend irsend;
    
    // Compute Hamming(7,4) error correction code for 4 data bits
    byte computeHamming(byte data) {
        byte code = 0;
        for (int i = 0; i < 4; i++) {
            if (data & (1 << i)) {
                code ^= HAMMING_MATRIX[i];
            }
        }
        return code;
    }
    
    // Encode single character into RC5 format with error correction
    unsigned int encodeCharacter(char c) {
        // Split character into two 4-bit chunks
        byte upperNibble = (c >> 4) & 0x0F;
        byte lowerNibble = c & 0x0F;
        
        // Compute Hamming codes
        byte upperHamming = computeHamming(upperNibble);
        byte lowerHamming = computeHamming(lowerNibble);
        
        // Combine into 14-bit RC5 code:
        // Format: [1-bit start][1-bit field][5-bit upper hamming][3-bit upper data][4-bit lower data]
        unsigned int rc5Code = 0x2000;  // Start bit (1) and field bit (0)
        rc5Code |= (upperHamming & 0x1F) << 7;
        rc5Code |= (upperNibble & 0x07) << 4;
        rc5Code |= (lowerNibble & 0x0F);
        
        return rc5Code;
    }

public:
    WiFiIRTransmitter() {
        IrSender.begin(IR_LED_PIN);
    }
    
    // Transmit WiFi credentials
    void transmitCredentials(const char* ssid, const char* password) {
        // Send start sequence
        sendStartSequence();
        
        // Send SSID
        for (size_t i = 0; i < strlen(ssid); i++) {
            transmitCharacter(ssid[i]);
        }
        
        // Send separator
        transmitCharacter('\n');
        
        // Send password
        for (size_t i = 0; i < strlen(password); i++) {
            transmitCharacter(password[i]);
        }
        
        // Send end sequence
        sendEndSequence();
    }
    
    // Transmit single character with retries
    void transmitCharacter(char c) {
        unsigned int code = encodeCharacter(c);
        
        for (int i = 0; i < RETRY_COUNT; i++) {
            IrSender.sendRC5(code, COMMAND_BITS);
            delay(CHAR_DELAY);
        }
    }
    
    // Send start sequence to indicate beginning of transmission
    void sendStartSequence() {
        for (int i = 0; i < 3; i++) {
            IrSender.sendRC5(0x3FFF, COMMAND_BITS);  // All 1s
            delay(CHAR_DELAY);
        }
    }
    
    // Send end sequence to indicate end of transmission
    void sendEndSequence() {
        for (int i = 0; i < 3; i++) {
            IrSender.sendRC5(0x2000, COMMAND_BITS);  // Only start bit
            delay(CHAR_DELAY);
        }
    }
};

// Example usage
void setup() {
    WiFiIRTransmitter transmitter;
    const char* ssid = "MyWiFiNetwork";
    const char* password = "MySecurePassword123";
    
    transmitter.transmitCredentials(ssid, password);
}

void loop() {
    // Empty loop
}
