import socket
import struct
from pathlib import Path
from uuid import UUID

from PIL import Image

from hwdaemon.ImageResult import ImageResult


class ImageFrameReceiver:
    MAX_FRAME_BYTES = 256 * 1024 * 1024

    def __init__(self, *, connection: socket.socket) -> None:
        self.connection = connection

    def save_next_frame(self, *, output_directory: Path) -> Path:
        (payload_length,) = struct.unpack("<I", self._read_exactly(length=4))
        if not 8 <= payload_length <= self.MAX_FRAME_BYTES:
            raise ValueError(f"Invalid image frame length: {payload_length}")
        payload = self._read_exactly(length=payload_length)
        if not ImageResult.ImageResultBufferHasIdentifier(payload, 0):
            raise ValueError("Expected OSSP image frame")
        image = ImageResult.GetRootAs(payload)
        metadata = image.Metadata()
        if metadata is None:
            raise ValueError("Frame has no image metadata")
        image_id = UUID(metadata.ImageId().decode())
        width, height = metadata.Width(), metadata.Height()
        if width <= 0 or height <= 0 or image.RawBytesIsNone():
            raise ValueError("Frame has no pixels or invalid dimensions")
        pixels = image.RawBytesAsNumpy().tobytes()
        pixel_count = width * height
        if len(pixels) == pixel_count:
            preview = Image.frombytes("L", (width, height), pixels)
        elif len(pixels) == pixel_count * 2:
            preview = Image.frombytes("I;16", (width, height), pixels, "raw", "I;16")
        elif len(pixels) == pixel_count * 3:
            preview = Image.frombytes("RGB", (width, height), pixels, "raw", "BGR")
        else:
            raise ValueError(f"Unsupported pixel layout: {len(pixels)} bytes for {width}x{height}")
        output_directory.mkdir(parents=True, exist_ok=True)
        destination = output_directory / f"{image_id}.png"
        preview.save(destination)
        print(
            f"Image {image_id}: {width}x{height}, {metadata.BitDepth()} bits, "
            f"{metadata.ExposureSeconds()}s; saved {destination}"
        )
        return destination

    def _read_exactly(self, *, length: int) -> bytearray:
        payload = bytearray(length)
        remaining = memoryview(payload)
        while remaining:
            received = self.connection.recv_into(remaining)
            if received == 0:
                raise EOFError("Image stream ended before a complete frame was received")
            remaining = remaining[received:]
        return payload
