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

## Symmetric Encryption with SD Key Cards
For high-security requirements, the project supports using SD cards as "key cards" containing raw random data (One-Time Pad style).

- **Raw Block Access**: Keys are read directly from SD card blocks without a filesystem.
- **Symmetric XOR Cipher**: Credentials and key evolution data are encrypted with a 512-byte key from the SD card.
- **Key Evolution (Poly Expansion)**: After each successful transmission, the used key block is replaced with a new one generated from a "compact crypto poly expansion" (8 GF(2^8) polynomial coefficients) sent with the message.
- **Life Extension (Wrap-around)**: When the end of the SD codebook is reached, the system wraps around to Block 1. Because every block was evolved during its first use, the "second lap" uses entirely new keys derived from the polynomial expansions, effectively extending the system's life indefinitely.
- **Faster Decline for Attackers**: If an attacker misses a transmission, they fail to capture the coefficients needed to evolve that block. When the system eventually wraps around, the attacker's original codebook will contain stale data for that block, causing decryption to fail. The more transmissions an attacker misses, the faster their ability to decrypt subsequent laps declines.
- **Out-of-Sync Protection**: The system relies on synchronized block indices and rolling codes (stored in EEPROM). Each transmission includes an encrypted sync header to allow the receiver to automatically resync to the transmitter's current block.

### Using SD Key Cards
1. Generate an initial key card image: `python3 generate_key_card.py --size 1`
2. Write `keycard.img` to two identical SD cards using `dd` or similar.
3. Use `ir_wifi_blaster_sd.ino` and `ir_wifi_receiver_sd.ino`.
4. Connect an SD card module to your Arduino/ESP (CS on pin 4 for Arduino, 15 for ESP8266, 5 for ESP32).

## Files
- `ir_wifi_blaster.ino` / `ir_wifi_receiver.ino`: Basic version with Hamming ECC and CRC.
- `ir_wifi_blaster_sd.ino` / `ir_wifi_receiver_sd.ino`: High-security SD-based version.
- `crypto.h`: GF(2^8) math and polynomial expansion logic.
- `hamming.h`: Shared Hamming(7,4) tables.
- `hamming_generator.py`: Script to generate Hamming tables.
- `generate_key_card.py`: Script to generate initial random SD images.

## TODO
- [ ] Implement a rolling code or timestamp for the basic version.
- [ ] Create a simple mobile app or web interface for the blaster.
