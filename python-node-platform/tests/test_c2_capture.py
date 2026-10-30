from uuid import UUID

import pytest

from c2_capture_example import C2CaptureExample
from example_clients import ExampleClients


TASK_ID = "00000000-0000-0000-0000-000000000002"
IMAGE_ID = "00000000-0000-0000-0000-000000000003"


def capture_task(status, result=None):
    return {
        "lineageId": "00000000-0000-0000-0000-000000000001",
        "task": {
            "id": TASK_ID, "createdAt": "2026-10-29T00:00:00Z",
            "updatedAt": "2026-10-29T00:00:00Z", "status": status,
            "type": "CAPTURE_IMAGE", "statusDetail": "camera unavailable", "result": result,
        },
    }


def test_polls_exact_task_and_reports_node_local_image(configuration, http_server, capsys):
    http_server.responses.extend([
        {"tasks": [capture_task("EXECUTING")]},
        {"tasks": [capture_task("SUCCEEDED", {"id": IMAGE_ID})], "offset": 0},
    ])
    with ExampleClients(configuration=configuration) as clients:
        image_id = C2CaptureExample(
            configuration=configuration, c2=clients.c2,
        ).run()
    assert image_id == UUID(IMAGE_ID)
    assert f"Captured image {IMAGE_ID}; file stored locally on the node" in capsys.readouterr().out
    assert list(configuration.output_directory.iterdir()) == []
    capture, poll = http_server.requests
    assert capture.path == "/c2/v1/camera/capture-image"
    assert capture.body["opticalTrains"] == [{
        "lineageId": str(configuration.lineage_id), "opticalTrainLabel": "astro",
    }]
    assert capture.body["request"] == {
        "exposure": 1.0, "gain": 0, "binning": 1, "offset": 0, "gainMode": 1,
    }
    assert poll.path == "/c2/v1/tasks"
    assert poll.body["ids"] == [TASK_ID]
    assert poll.body["lineageIds"] == [str(configuration.lineage_id)]
    for request in http_server.requests:
        assert request.headers["Authorization"] == "Bearer test-token"


@pytest.mark.parametrize("status", ["FAILED", "CANCELLED", "MISSED"])
def test_unsuccessful_task_reports_details_without_downloading(configuration, http_server, status):
    http_server.responses.append({"tasks": [capture_task(status)]})
    with ExampleClients(configuration=configuration) as clients:
        with pytest.raises(RuntimeError, match=f"{status}: camera unavailable"):
            C2CaptureExample(
                configuration=configuration, c2=clients.c2,
            ).run()
    assert len(http_server.requests) == 1


def test_timeout_reports_task_may_still_be_running(configuration, http_server):
    configuration.timeout_seconds = 0.01
    http_server.responses.append({"tasks": [capture_task("EXECUTING")]})
    with ExampleClients(configuration=configuration) as clients:
        with pytest.raises(TimeoutError, match=f"{TASK_ID}; the cloud task may still be running"):
            C2CaptureExample(
                configuration=configuration, c2=clients.c2,
            ).run()
    assert len(http_server.requests) == 1


def test_success_without_image_id_does_not_download(configuration, http_server):
    http_server.responses.append({"tasks": [capture_task("SUCCEEDED")]})
    with ExampleClients(configuration=configuration) as clients:
        with pytest.raises(ValueError, match="Capture result has no image id"):
            C2CaptureExample(
                configuration=configuration, c2=clients.c2,
            ).run()
    assert len(http_server.requests) == 1
