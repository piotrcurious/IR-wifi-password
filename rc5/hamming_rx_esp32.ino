#include <IRremote.hpp>
#include <WiFi.h>
#include <esp_wifi.h>

// Pin configuration
const int IR_RECV_PIN = 27;  // IR receiver module pin
const int STATUS_LED = 2;    // Built-in LED pin on most ESP32 dev boards

// RC5 protocol constants
const unsigned int COMMAND_BITS = 14;
const unsigned long TIMEOUT = 10000;  // Timeout for receiving complete credentials (10 seconds)
const int MAX_CREDENTIAL_LENGTH = 32; // Maximum length for SSID or password

// Hamming(7,4) decoding matrix - for error correction
const byte HAMMING_CHECK_MATRIX[3] = {0b1010101, 0b0110011, 0b0001111};

class WiFiIRReceiver {
private:
    char ssid[MAX_CREDENTIAL_LENGTH + 1];
    char password[MAX_CREDENTIAL_LENGTH + 1];
    int ssidLength = 0;
    int passwordLength = 0;
    bool isReceivingPassword = false;
    
    // Decode Hamming(7,4) code and correct single-bit errors
    byte decodeHamming(byte code) {
        // Calculate syndrome
        byte syndrome = 0;
        for (int i = 0; i < 3; i++) {
            byte parity = 0;
            for (int j = 0; j < 7; j++) {
                if ((code & (1 << j)) && (HAMMING_CHECK_MATRIX[i] & (1 << j))) {
                    parity ^= 1;
                }
            }
            syndrome |= (parity << i);
        }
        
        // Correct single-bit errors
        if (syndrome != 0) {
            code ^= (1 << (syndrome - 1));
        }
        
        // Extract data bits
        byte data = 0;
        data |= (code & 0x08) >> 3;
        data |= (code & 0x04) >> 1;
        data |= (code & 0x02) << 1;
        data |= (code & 0x01) << 3;
        
        return data;
    }
    
    // Decode RC5 code back to character
    char decodeCharacter(uint16_t rc5Code) {
        // Extract Hamming codes and data
        byte upperHamming = (rc5Code >> 7) & 0x1F;
        byte upperNibble = (rc5Code >> 4) & 0x07;
        byte lowerNibble = rc5Code & 0x0F;
        
        // Decode upper nibble using Hamming code
        byte correctedUpper = decodeHamming((upperHamming << 3) | upperNibble);
        
        // Combine nibbles into character
        return (correctedUpper << 4) | lowerNibble;
    }
    
    // Check if code is start sequence
    bool isStartSequence(uint16_t code) {
        return code == 0x3FFF;
    }
    
    // Check if code is end sequence
    bool isEndSequence(uint16_t code) {
        return code == 0x2000;
    }
    
public:
    WiFiIRReceiver() {
        IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
    }
    
    // Receive and process WiFi credentials
    bool receiveCredentials() {
        unsigned long startTime = millis();
        int startSequenceCount = 0;
        int endSequenceCount = 0;
        
        Serial.println("Waiting for IR transmission...");
        
        while ((millis() - startTime) < TIMEOUT) {
            if (IrReceiver.decode()) {
                uint16_t receivedCode = IrReceiver.decodedIRData.decodedRawData;
                IrReceiver.resume();
                
                // Check for start sequence
                if (isStartSequence(receivedCode)) {
                    startSequenceCount++;
                    if (startSequenceCount == 3) {
                        Serial.println("Start sequence detected");
                        startTime = millis();  // Reset timeout
                        continue;
                    }
                }
                
                // Check for end sequence
                if (isEndSequence(receivedCode)) {
                    endSequenceCount++;
                    if (endSequenceCount == 3) {
                        Serial.println("End sequence detected");
                        return true;
                    }
                    continue;
                }
                
                // Process regular character
                if (startSequenceCount >= 3) {
                    char receivedChar = decodeCharacter(receivedCode);
                    
                    // Check for separator between SSID and password
                    if (receivedChar == '\n') {
                        isReceivingPassword = true;
                        continue;
                    }
                    
                    // Store character in appropriate buffer
                    if (!isReceivingPassword && ssidLength < MAX_CREDENTIAL_LENGTH) {
                        ssid[ssidLength++] = receivedChar;
                        Serial.print(".");  // Progress indicator
                    } else if (isReceivingPassword && passwordLength < MAX_CREDENTIAL_LENGTH) {
                        password[passwordLength++] = receivedChar;
                        Serial.print("*");  // Progress indicator
                    }
                }
            }
            vTaskDelay(1);  // Allow other tasks to run
        }
        
        return false;  // Timeout occurred
    }
    
    // Connect to WiFi using received credentials
    bool connectToWiFi() {
        if (ssidLength == 0 || passwordLength == 0) {
            Serial.println("No credentials received");
            return false;
        }
        
        Serial.printf("\nAttempting to connect to SSID: %s\n", ssid);
        
        // Disable auto-reconnect and disconnect from any previous network
        WiFi.persistent(false);
        WiFi.disconnect(true);
        
        // Set WiFi mode and begin connection
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        
        // Wait for connection with timeout
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Connected successfully! IP: %s\n", WiFi.localIP().toString().c_str());
            // Optional: Save good credentials to NVS
            return true;
        } else {
            Serial.println("Failed to connect");
            return false;
        }
    }
};

// Task handle for LED blinking
TaskHandle_t blinkTaskHandle = NULL;

// LED blinking patterns
void blinkTask(void *parameter) {
    int pattern = (int)parameter;
    while (true) {
        switch (pattern) {
            case 0: // Waiting pattern - slow blink
                digitalWrite(STATUS_LED, HIGH);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                digitalWrite(STATUS_LED, LOW);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                break;
                
            case 1: // Success pattern - rapid blink
                for (int i = 0; i < 5; i++) {
                    digitalWrite(STATUS_LED, HIGH);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    digitalWrite(STATUS_LED, LOW);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
                
            case 2: // Error pattern - three long blinks
                for (int i = 0; i < 3; i++) {
                    digitalWrite(STATUS_LED, HIGH);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    digitalWrite(STATUS_LED, LOW);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
                
            case 3: // Connected pattern - solid on
                digitalWrite(STATUS_LED, HIGH);
                vTaskDelay(portMAX_DELAY);
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(STATUS_LED, OUTPUT);
    
    // Start LED blinking task for waiting pattern
    xTaskCreate(blinkTask, "BlinkTask", 2048, (void*)0, 1, &blinkTaskHandle);
    
    WiFiIRReceiver receiver;
    bool success = false;
    
    while (!success) {
        // Try to receive credentials
        if (receiver.receiveCredentials()) {
            // Update LED pattern to success
            vTaskDelete(blinkTaskHandle);
            xTaskCreate(blinkTask, "BlinkTask", 2048, (void*)1, 1, &blinkTaskHandle);
            
            // Try to connect to WiFi
            if (receiver.connectToWiFi()) {
                // Update LED pattern to connected
                vTaskDelete(blinkTaskHandle);
                xTaskCreate(blinkTask, "BlinkTask", 2048, (void*)3, 1, &blinkTaskHandle);
                success = true;
            } else {
                // Update LED pattern to error
                vTaskDelete(blinkTaskHandle);
                xTaskCreate(blinkTask, "BlinkTask", 2048, (void*)2, 1, &blinkTaskHandle);
                delay(2000);
                // Restart waiting pattern
                vTaskDelete(blinkTaskHandle);
                xTaskCreate(blinkTask, "BlinkTask", 2048, (void*)0, 1, &blinkTaskHandle);
            }
        }
    }
}

void loop() {
    // Empty loop - device is configured
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
