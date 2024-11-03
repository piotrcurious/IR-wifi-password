def hamming74_decode_table():
    # Hamming (7,4) encoding table with no errors for reference
    hamming_codes = {
        0b0000000: 0x0, 0b0001011: 0x1, 0b0010110: 0x2, 0b0011101: 0x3,
        0b0100111: 0x4, 0b0101100: 0x5, 0b0110001: 0x6, 0b0111010: 0x7,
        0b1000101: 0x8, 0b1001110: 0x9, 0b1010011: 0xA, 0b1011000: 0xB,
        0b1100010: 0xC, 0b1101001: 0xD, 0b1110100: 0xE, 0b1111111: 0xF
    }

    # Initialize table with error value (0xFF)
    decode_table = [0xFF] * 128

    # Populate table for exact matches and single-bit error correction
    for code, nibble in hamming_codes.items():
        # Set exact code mapping
        decode_table[code] = nibble
        # Set mappings for each single-bit error
        for bit in range(7):
            error_code = code ^ (1 << bit)
            decode_table[error_code] = nibble

    return decode_table

def format_table_for_arduino(table):
    # Format the table as a C-style array for Arduino code
    formatted = "const uint8_t hamming74_decode[128] PROGMEM = {\n    "
    for i, value in enumerate(table):
        formatted += f"0x{value:02X}, "
        if (i + 1) % 8 == 0:  # New line every 8 entries
            formatted += "\n    "
    formatted = formatted.rstrip(", \n") + "\n};"
    return formatted

# Generate and format the table
decode_table = hamming74_decode_table()
arduino_code = format_table_for_arduino(decode_table)
print(arduino_code)
