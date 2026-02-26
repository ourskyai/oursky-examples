#!/usr/bin/env python3
"""Convert a sample InceptionV3 JPEG to a 3-channel FITS file.

Usage: python3 generate_test_fits.py <output_path>

Finds a sample JPEG from the SNPE SDK's InceptionV3 example data and
writes it as a 3-channel (RGB) FITS file. Requires Pillow (pip install Pillow).
"""
import glob
import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
SAMPLE_IMAGE_DIR = os.path.join(
    PROJECT_ROOT,
    "third-party", "snpe-sdk", "*",
    "examples", "Models", "InceptionV3", "data", "cropped",
)


def find_sample_jpeg():
    """Find a cropped sample JPEG from the SNPE SDK."""
    candidates = glob.glob(os.path.join(SAMPLE_IMAGE_DIR, "*.jpg"))
    if not candidates:
        return None
    return candidates[0]


def write_fits_rgb(path, pixels, width, height):
    """Write a 3-channel FITS image from flat RGB float data.

    FITS stores multi-channel data as sequential planes:
    [R plane][G plane][B plane], each width*height big-endian float32.
    """
    cards = [
        "SIMPLE  =                    T / Standard FITS format",
        "BITPIX  =                  -32 / 32-bit IEEE float",
        "NAXIS   =                    3 / 3D data cube (RGB)",
        f"NAXIS1  =          {width:>10d} / Image width",
        f"NAXIS2  =          {height:>10d} / Image height",
        "NAXIS3  =                    3 / RGB channels",
        "END",
    ]

    header = b""
    for card in cards:
        header += card.ljust(80).encode("ascii")

    remainder = len(header) % 2880
    if remainder:
        header += b" " * (2880 - remainder)

    # pixels is [R0,G0,B0, R1,G1,B1, ...] (HWC interleaved)
    # FITS wants plane-sequential: all R, then all G, then all B
    n = width * height
    pixel_data = b""
    for c in range(3):
        for i in range(n):
            pixel_data += struct.pack(">f", pixels[i * 3 + c])

    remainder = len(pixel_data) % 2880
    if remainder:
        pixel_data += b"\0" * (2880 - remainder)

    with open(path, "wb") as f:
        f.write(header)
        f.write(pixel_data)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <output_path>", file=sys.stderr)
        sys.exit(1)

    out_path = sys.argv[1]

    try:
        from PIL import Image
    except ImportError:
        print("Pillow is required: pip install Pillow", file=sys.stderr)
        sys.exit(1)

    jpeg_path = find_sample_jpeg()
    if not jpeg_path:
        print("No sample JPEG found in SNPE SDK InceptionV3 data.", file=sys.stderr)
        print(f"Searched: {SAMPLE_IMAGE_DIR}/*.jpg", file=sys.stderr)
        sys.exit(1)

    print(f"Converting {os.path.basename(jpeg_path)} to FITS...")
    img = Image.open(jpeg_path).convert("RGB")

    width, height = img.size
    raw = img.tobytes()

    # Normalize to [0, 1] float
    pixels = [b / 255.0 for b in raw]

    write_fits_rgb(out_path, pixels, width, height)
    print(f"Wrote {width}x{height}x3 FITS image to {out_path}")


if __name__ == "__main__":
    main()
