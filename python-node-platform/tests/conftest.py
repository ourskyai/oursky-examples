import json
import threading
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from types import SimpleNamespace
from urllib.parse import parse_qs, urlsplit

import pytest
import flatbuffers

from example_configuration import ExampleConfiguration
from hwdaemon import ImageMetadata, ImageResult


@pytest.fixture
def http_server():
    responses = deque()
    requests = []

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            self.respond()

        def do_POST(self):
            self.respond()

        def do_PUT(self):
            self.respond()

        def respond(self):
            url = urlsplit(self.path)
            body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
            request = SimpleNamespace(
                method=self.command, path=url.path, query=parse_qs(url.query),
                headers=self.headers, body=json.loads(body) if body else None,
            )
            requests.append(request)
            response = responses.popleft()
            if callable(response):
                response = response(request)
            status, body = response if isinstance(response, tuple) else (200, response)
            content_type = "application/octet-stream" if isinstance(body, bytes) else "application/json"
            payload = body if isinstance(body, bytes) else json.dumps(body).encode()
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

        def log_message(self, *arguments):
            pass

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield SimpleNamespace(
            base_url=f"http://127.0.0.1:{server.server_port}",
            responses=responses, requests=requests,
        )
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


@pytest.fixture
def configuration(http_server, tmp_path):
    return ExampleConfiguration(environment={
        "OS_API_TOKEN": "test-token",
        "OS_LINEAGE_ID": "00000000-0000-0000-0000-000000000001",
        "OS_API_BASE_URL": http_server.base_url,
        "OS_OPTICAL_TRAIN_LABEL": "astro",
        "OS_OUTPUT_DIRECTORY": str(tmp_path),
        "OS_TIMEOUT_SECONDS": "5",
        "OS_STREAM_RECEIVER_HOST": "127.0.0.1",
        "OS_STREAM_RECEIVER_PORT": "0",
        "OS_FRAME_COUNT": "1",
        "OS_METRICS_SECONDS": "1",
    })


@pytest.fixture
def image_frame():
    def build(*, pixels=b"\x0c\x22", width=2, height=1, bit_depth=8,
              image_id="00000000-0000-0000-0000-000000000003", identifier=b"OSSP"):
        builder = flatbuffers.Builder(256)
        raw_bytes = builder.CreateByteVector(pixels)
        identifier_string = builder.CreateString(image_id)
        ImageMetadata.Start(builder)
        ImageMetadata.AddImageId(builder, identifier_string)
        ImageMetadata.AddWidth(builder, width)
        ImageMetadata.AddHeight(builder, height)
        ImageMetadata.AddBitDepth(builder, bit_depth)
        ImageMetadata.AddExposureSeconds(builder, 1.0)
        metadata = ImageMetadata.End(builder)
        ImageResult.Start(builder)
        ImageResult.AddRawBytes(builder, raw_bytes)
        ImageResult.AddMetadata(builder, metadata)
        image = ImageResult.End(builder)
        builder.FinishSizePrefixed(image, file_identifier=identifier)
        return bytes(builder.Output())

    return build
