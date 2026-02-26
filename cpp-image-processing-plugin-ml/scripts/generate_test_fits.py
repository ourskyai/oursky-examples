#!/usr/bin/env python3
"""Generate a minimal FITS test image using only the Python stdlib.

Usage: python3 generate_test_fits.py <output_path> [width] [height]

Creates a 2D grayscale FITS file with a diagonal gradient pattern.
Default size is 512x512 pixels.
"""
import struct
import sys


def write_fits(path, width, height):
    # Build header cards (each exactly 80 bytes, ASCII)
    cards = [
        "SIMPLE  =                    T / Standard FITS format",
        f"BITPIX  =                  -32 / 32-bit IEEE float",
        f"NAXIS   =                    2 / 2D image",
        f"NAXIS1  =          {width:>10d} / Image width",
        f"NAXIS2  =          {height:>10d} / Image height",
        "END",
    ]

    header = b""
    for card in cards:
        header += card.ljust(80).encode("ascii")

    # Pad header to multiple of 2880 bytes
    remainder = len(header) % 2880
    if remainder:
        header += b" " * (2880 - remainder)

    # Generate a diagonal gradient pattern (values 0.0 .. 1.0)
    # Big-endian float32 as required by FITS standard
    pixel_data = b""
    for y in range(height):
        for x in range(width):
            val = (x + y) / max(width + height - 2, 1)
            pixel_data += struct.pack(">f", val)

    # Pad data to multiple of 2880 bytes
    remainder = len(pixel_data) % 2880
    if remainder:
        pixel_data += b"\0" * (2880 - remainder)

    with open(path, "wb") as f:
        f.write(header)
        f.write(pixel_data)

    print(f"Wrote {width}x{height} FITS image to {path}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <output_path> [width] [height]", file=sys.stderr)
        sys.exit(1)

    out_path = sys.argv[1]
    w = int(sys.argv[2]) if len(sys.argv) > 2 else 512
    h = int(sys.argv[3]) if len(sys.argv) > 3 else 512
    write_fits(out_path, w, h)
