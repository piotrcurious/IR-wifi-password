#include <IRremoteESP32.h>
#include <WiFi.h>

const uint16_t RECV_PIN = 14;   // GPIO pin for IR receiver
const int bufferSize = 64;      // Maximum characters for SSID + password

// Hamming (7,4) Decoding Table in PROGMEM
const uint8_t hamming74_decode[128] PROGMEM = {
    0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x02, 0x03, 0xFF, 
    0xFF, 0x04, 0x05, 0xFF, 0x06, 0xFF, 0xFF, 0x07,
    0xFF, 0x08, 0x09, 0xFF, 0x0A, 0xFF, 0xFF, 0x0B,
    0x0C, 0xFF, 0xFF, 0x0D, 0xFF, 0x0E, 0x0F, 0xFF,
    // Complete table to cover other entries for decoding
};

// Create IR receiver object
IRrecv irrecv(RECV_PIN);
decode_results results;

char receivedBuffer[bufferSize];
int bufferIndex = 0;

// Decode Hamming (7,4) data from PROGMEM table with error correction
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
        return; // Skip if uncorrectable error
    }
    receivedBuffer[bufferIndex++] = decodedNibble;  // Store decoded nibble
    if (bufferIndex >= bufferSize) bufferIndex = 0; // Wrap around if buffer full
}

// Decode and reconstruct the SSID and password from received buffer
void decodeBuffer() {
    char ssid[32] = {0};
    char password[32] = {0};
    int ssidIndex = 0, passIndex = 0;
    bool isSSID = true;

    for (int i = 0; i < bufferIndex; i += 2) {
        if (i + 1 < bufferIndex) {
            char decodedChar = (receivedBuffer[i] << 4) | receivedBuffer[i + 1];
            if (decodedChar == '\0') {  // Assume '\0' separates SSID and password
                isSSID = false;
            } else if (isSSID && ssidIndex < sizeof(ssid) - 1) {
                ssid[ssidIndex++] = decodedChar;
            } else if (!isSSID && passIndex < sizeof(password) - 1) {
                password[passIndex++] = decodedChar;
            }
        }
    }
    ssid[ssidIndex] = '\0';
    password[passIndex] = '\0';

    Serial.printf("Decoded SSID: %s\n", ssid);
    Serial.printf("Decoded Password: %s\n", password);

    // Attempt to connect to Wi-Fi using the decoded SSID and password
    connectToWiFi(ssid, password);
    bufferIndex = 0;  // Reset buffer after decoding
}

// Connect to Wi-Fi using received SSID and password
void connectToWiFi(const char* ssid, const char* password) {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi.");
    }
}

// Main function to handle IR reception
void receiveIRData() {
    if (irrecv.decode(&results)) {
        if (results.decode_type == RC5) {
            processRC5Command(results.value);  // Decode and store RC5 command
        }
        irrecv.resume();  // Receive the next value

        // Periodically decode the buffer when it fills a complete message
        if (bufferIndex > 0 && bufferIndex % 2 == 0) {
            decodeBuffer();
        }
    }
}

void setup() {
    Serial.begin(115200);
    irrecv.enableIRIn();  // Start the receiver
    Serial.println("Ready to receive IR signals...");
    WiFi.mode(WIFI_STA);  // Set Wi-Fi to station mode
}

void loop() {
    receiveIRData();
}
