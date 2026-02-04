def hamming74_tables():
    # Hamming (7,4) encoding table with no errors for reference
    # Data bits d4 d3 d2 d1, Codeword d4 d3 d2 d1 p3 p2 p1
    # p1 = d1 ^ d3 ^ d4
    # p2 = d1 ^ d2 ^ d3
    # p3 = d1 ^ d2 ^ d4

    encode_table = []
    hamming_codes = {}
    for nibble in range(16):
        d1 = (nibble >> 0) & 1
        d2 = (nibble >> 1) & 1
        d3 = (nibble >> 2) & 1
        d4 = (nibble >> 3) & 1

        p1 = d1 ^ d3 ^ d4
        p2 = d1 ^ d2 ^ d3
        p3 = d1 ^ d2 ^ d4

        code = (d4 << 6) | (d3 << 5) | (d2 << 4) | (d1 << 3) | (p3 << 2) | (p2 << 1) | p1
        encode_table.append(code)
        hamming_codes[code] = nibble

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

    return encode_table, decode_table

def format_table_for_arduino(name, table, type="uint8_t"):
    formatted = f"const {type} {name}[{len(table)}] PROGMEM = {{\n    "
    for i, value in enumerate(table):
        formatted += f"0x{value:02X}, "
        if (i + 1) % 8 == 0:  # New line every 8 entries
            formatted += "\n    "
    formatted = formatted.rstrip(", \n") + "\n};"
    return formatted

# Generate and format the tables
encode_table, decode_table = hamming74_tables()
print(format_table_for_arduino("hamming74_encode", encode_table))
print()
print(format_table_for_arduino("hamming74_decode", decode_table))
