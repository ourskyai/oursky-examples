import socket
from contextlib import ExitStack
from urllib.parse import urlsplit

import pytest
from ourskyai_node_platform_api import ApiException

from continuous_capture_example import ContinuousCaptureExample
from example_clients import ExampleClients


@pytest.fixture
def image_stream(http_server, image_frame):
    with ExitStack() as connections:
        state = {"payload": image_frame(), "cleanup_status": 200, "capture_status": 200}

        def start_stream(request):
            receiver = urlsplit(request.body["streamReceiverUrl"])
            state["connection"] = connections.enter_context(socket.create_connection(
                (receiver.hostname, receiver.port), timeout=5,
            ))
            return {}

        def start_capture(request):
            state["connection"].sendall(state["payload"])
            return state["capture_status"], {}

        def stop(request):
            return state["cleanup_status"], {}

        http_server.responses.extend([start_stream, start_capture, stop, stop])
        yield state


def run_capture(configuration):
    with ExampleClients(configuration=configuration) as clients:
        ContinuousCaptureExample(
            configuration=configuration, node_platform=clients.node_platform,
        ).run()


def test_capture_saves_frames_and_stops_same_optical_train(configuration, http_server, image_stream):
    run_capture(configuration)
    assert len(list(configuration.output_directory.glob("*.png"))) == 1
    start_stream, capture, stop_capture, stop_stream = http_server.requests
    assert start_stream.path.endswith("/start-image-stream")
    assert capture.path.endswith("/capture-images-continuously")
    assert stop_capture.path.endswith("/stop-capturing-images-continuously")
    assert stop_stream.path.endswith("/stop-image-stream")
    assert capture.body["enableFileSaving"] is False
    assert capture.query["opticalTrainLabel"] == stop_capture.query["opticalTrainLabel"] == ["astro"]
    for request in http_server.requests:
        assert request.query["lineageId"] == [str(configuration.lineage_id)]
        assert request.headers["Authorization"] == "Bearer test-token"


def test_requested_frame_count_bounds_capture(configuration, image_stream, image_frame):
    configuration.environment["OS_FRAME_COUNT"] = "2"
    image_stream["payload"] = b"".join(
        image_frame(image_id=f"00000000-0000-0000-0000-{number:012d}")
        for number in range(1, 4)
    )
    run_capture(configuration)
    assert len(list(configuration.output_directory.glob("*.png"))) == 2


def test_receive_error_is_retained_when_both_stops_fail(
    configuration, http_server, image_stream, caplog,
):
    image_stream["payload"] = bytes(4)
    image_stream["cleanup_status"] = 500
    with pytest.raises(ValueError, match="Invalid image frame length"):
        run_capture(configuration)
    assert len(http_server.requests) == 4
    assert "Failed to stop capture or streaming" in caplog.text


def test_cleanup_failure_surfaces_after_successful_capture(configuration, http_server, image_stream):
    image_stream["cleanup_status"] = 500
    with pytest.raises(ApiException):
        run_capture(configuration)
    assert len(http_server.requests) == 4


def test_capture_start_failure_still_stops_stream(configuration, http_server, image_stream):
    image_stream["capture_status"] = 500
    with pytest.raises(ApiException):
        run_capture(configuration)
    assert len(http_server.requests) == 3
    assert http_server.requests[-1].path.endswith("/stop-image-stream")


def test_accept_timeout_still_stops_capture_and_stream(configuration, http_server):
    configuration.timeout_seconds = 0.1
    http_server.responses.extend([{}, {}, {}, {}])
    with pytest.raises(TimeoutError):
        run_capture(configuration)
    assert len(http_server.requests) == 4
