# IR-wifi-password
Sending WiFi SSID/password over IR-serial protocol.

This project allows "pairing" WiFi devices by putting them close together and exchanging credentials using Infrared.

## Features
- **Robust Transmission**: Uses Hamming(7,4) error correction for every nibble sent.
- **Reliable Protocol**: Includes preamble, length headers, and CRC8 checksum to ensure data integrity.
- **Multi-platform**: Supports Arduino (Atmel AVR), ESP8266, and ESP32.
- **Secure-ish**: Close-proximity IR is naturally limited in range, providing a layer of physical security.

## Protocol Details
The data is sent as a series of 14-bit RC5 words:
1. **Preamble**: `0x3FFF` sent 3 times.
2. **SSID Length**: 1 byte (encoded as 2 Hamming nibbles).
3. **Password Length**: 1 byte (encoded as 2 Hamming nibbles).
4. **SSID**: N bytes (each encoded as 2 Hamming nibbles).
5. **Password**: M bytes (each encoded as 2 Hamming nibbles).
6. **CRC8**: 1 byte (encoded as 2 Hamming nibbles).

Each Hamming nibble is wrapped in a 14-bit RC5 word with bit 13 set to 1 to ensure a start bit:
`Word = 0x2000 | Hamming74(nibble)`

## Usage

### Transmitter (Blaster)
1. Open `ir_wifi_blaster.ino` in Arduino IDE.
2. Set your `ssid` and `password` in the code.
3. Upload to an Arduino (e.g., Uno, Nano) with an IR LED connected to pin 3.

### Receiver
1. Open `ir_wifi_receiver.ino` in Arduino IDE.
2. Ensure you have the `IRremote` library (v4.x) installed.
3. Upload to an ESP8266 or ESP32 with an IR receiver module (e.g., TSOP38238) connected to the specified pin (D5 on ESP8266, GPIO15 on ESP32 by default).
4. Open Serial Monitor at 115200 baud.
5. Bring the transmitter close to the receiver.

## Files
- `ir_wifi_blaster.ino`: Transmitter sketch.
- `ir_wifi_receiver.ino`: Receiver sketch.
- `hamming.h`: Shared Hamming(7,4) tables and helper functions.
- `hamming_generator.py`: Python script used to generate the Hamming tables.

## TODO
- [ ] Add symmetric encryption (e.g., Speck or XOR with a shared key).
- [ ] Implement a rolling code or timestamp to prevent replay attacks.
- [ ] Create a simple mobile app or web interface for the blaster.
