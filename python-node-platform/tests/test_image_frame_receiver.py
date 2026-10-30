import struct

import pytest
from PIL import Image

from image_frame_receiver import ImageFrameReceiver


class FragmentedConnection:
    def __init__(self, payload):
        self.payload = memoryview(payload)

    def recv_into(self, destination):
        length = min(3, len(destination), len(self.payload))
        destination[:length] = self.payload[:length]
        self.payload = self.payload[length:]
        return length


@pytest.mark.parametrize(("pixels", "bit_depth", "expected"), [
    (b"\x00\xff", 8, [0, 255]),
    (struct.pack("<HH", 256, 65535), 16, [256, 65535]),
    (bytes([10, 20, 30, 40, 50, 60]), 8, [(30, 20, 10), (60, 50, 40)]),
])
def test_fragmented_frames_preserve_png_samples(image_frame, tmp_path, pixels, bit_depth, expected):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(
        image_frame(pixels=pixels, bit_depth=bit_depth),
    ))
    destination = receiver.save_next_frame(output_directory=tmp_path)
    with Image.open(destination) as image:
        assert image.size == (2, 1)
        assert list(image.get_flattened_data()) == expected


def test_two_frames_on_one_connection(image_frame, tmp_path):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(
        image_frame() + image_frame(image_id="00000000-0000-0000-0000-000000000004"),
    ))
    first = receiver.save_next_frame(output_directory=tmp_path)
    second = receiver.save_next_frame(output_directory=tmp_path)
    assert first != second
    assert first.exists() and second.exists()


@pytest.mark.parametrize("length", [0, 7, ImageFrameReceiver.MAX_FRAME_BYTES + 1])
def test_invalid_length_is_rejected_before_allocating(tmp_path, length):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(struct.pack("<I", length)))
    with pytest.raises(ValueError, match="Invalid image frame length"):
        receiver.save_next_frame(output_directory=tmp_path)


def test_truncated_frame_fails(image_frame, tmp_path):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(image_frame()[:-1]))
    with pytest.raises(EOFError, match="complete frame"):
        receiver.save_next_frame(output_directory=tmp_path)


def test_wrong_identifier_fails(image_frame, tmp_path):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(image_frame(identifier=b"NOPE")))
    with pytest.raises(ValueError, match="Expected OSSP"):
        receiver.save_next_frame(output_directory=tmp_path)


def test_unsupported_pixel_layout_fails(image_frame, tmp_path):
    receiver = ImageFrameReceiver(connection=FragmentedConnection(image_frame(pixels=b"12345")))
    with pytest.raises(ValueError, match="Unsupported pixel layout"):
        receiver.save_next_frame(output_directory=tmp_path)
