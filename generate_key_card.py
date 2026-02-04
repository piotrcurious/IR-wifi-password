import os
import argparse

def generate_key_card(filename, size_mb):
    size_bytes = size_mb * 1024 * 1024
    print(f"Generating {size_mb}MB random key card image: {filename}")

    with open(filename, 'wb') as f:
        # Generate random data in chunks to avoid memory issues
        chunk_size = 1024 * 1024
        for _ in range(size_mb):
            f.write(os.urandom(chunk_size))

    print("Done. Use 'dd' (Linux/Mac) or Win32DiskImager (Windows) to write this to your SD card.")
    print(f"Example: sudo dd if={filename} of=/dev/sdX bs=4M status=progress")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a random key card image for IR-wifi-password")
    parser.add_argument("--out", default="keycard.img", help="Output filename")
    parser.add_argument("--size", type=int, default=1, help="Size in MB")

    args = parser.parse_args()
    generate_key_card(args.out, args.size)
