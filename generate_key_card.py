import os
import argparse
import struct

def generate_key_card(filename, size_mb):
    # SD block size is 512 bytes
    BLOCK_SIZE = 512
    total_blocks = (size_mb * 1024 * 1024) // BLOCK_SIZE

    print(f"Generating {size_mb}MB random key card image: {filename}")
    print(f"Total blocks: {total_blocks}")

    master_key = os.urandom(32)

    # Pack SDHeader (Block 0)
    # char magic[8], uint32_t totalBlocks, uint8_t masterKey[32], uint8_t reserved[468]
    header = struct.pack("<8sI32s468s", b"IRWIFI01", total_blocks, master_key, b"\x00" * 468)

    with open(filename, 'wb') as f:
        # Write Block 0 (Header)
        f.write(header)

        # Write the rest as random blocks
        chunk_size = 1024 * 1024
        # We already wrote 512 bytes, so we need size_mb * 1024 * 1024 - 512 bytes more
        # To keep it simple, we write (size_mb * 1024 * 1024) bytes of random data
        # but skip the first 512 bytes or just write total_blocks - 1 more blocks.
        for i in range(total_blocks - 1):
            if (i * BLOCK_SIZE) % chunk_size == 0:
                data = os.urandom(chunk_size)
                f.write(data[:BLOCK_SIZE])
                data_offset = BLOCK_SIZE
            else:
                f.write(data[data_offset:data_offset+BLOCK_SIZE])
                data_offset += BLOCK_SIZE

    print("Done. Header written to Block 0.")
    print(f"Master Key (hex): {master_key.hex()}")
    print("Use 'dd' or Win32DiskImager to write this to your SD card.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a random key card image for IR-wifi-password")
    parser.add_argument("--out", default="keycard.img", help="Output filename")
    parser.add_argument("--size", type=int, default=1, help="Size in MB")

    args = parser.parse_args()
    generate_key_card(args.out, args.size)
