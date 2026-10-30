import base64
import json
import threading
from types import SimpleNamespace

import pytest
from google.protobuf.message import DecodeError
from google.protobuf.timestamp_pb2 import Timestamp
from websockets.exceptions import ConnectionClosed
from websockets.sync.server import serve

from example_clients import ExampleClients
from generated.metrics_pb2 import COUNT, GAUGE, V1Metric, V2MetricBatch, V2MetricPoint, V2MetricSeries
from metrics_stream_example import MetricsStreamExample


@pytest.fixture
def metrics_server():
    state = SimpleNamespace(
        authorized=True, send_welcome=False, envelopes=[], received=[], close_after_subscription=False,
    )

    def handler(websocket):
        if state.send_welcome:
            websocket.send(json.dumps({"type": "welcome"}))
        state.received.append(json.loads(websocket.recv(timeout=5)))
        websocket.send(json.dumps({"type": "authorization", "authorized": state.authorized}))
        if state.authorized:
            state.received.append(json.loads(websocket.recv(timeout=5)))
            for envelope in state.envelopes:
                websocket.send(json.dumps(envelope))
            if state.close_after_subscription:
                websocket.close(1000, "Fixture closed")
                return
        for message in websocket:
            state.received.append(json.loads(message))

    with serve(handler, "127.0.0.1", 0) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        state.url = f"ws://127.0.0.1:{server.socket.getsockname()[1]}/"
        try:
            yield state
        finally:
            server.shutdown()
            thread.join(timeout=5)


def envelope(*, suffix, payload):
    return {
        "type": "nats", "subject": f"metrics.oursky.00000000-0000-0000-0000-000000000001.{suffix}",
        "data": base64.b64encode(payload).decode(),
    }


def run_metrics(configuration, http_server, metrics_server):
    configuration.environment["OS_METRICS_WEBSOCKET_URL"] = metrics_server.url
    http_server.responses.append({"token": "node-scoped-edge-token"})
    with ExampleClients(configuration=configuration) as clients:
        MetricsStreamExample(configuration=configuration, platform=clients.platform).run()


@pytest.mark.parametrize("send_welcome", [False, True])
def test_waits_for_authorization_before_subscription_and_decodes_all_samples(
    configuration, http_server, metrics_server, capsys, send_welcome,
):
    metrics_server.send_welcome = send_welcome
    timestamp = Timestamp(seconds=10, nanos=123)
    metric = V1Metric(
        name="camera.temperature", value=12.5, type=GAUGE,
        measured_at=timestamp, tags=["source:node_controller"],
    )
    batch = V2MetricBatch(series=[V2MetricSeries(
        name="camera.frames", type=COUNT, tags=["camera:astro"], points=[
            V2MetricPoint(measured_at=timestamp, value=2),
            V2MetricPoint(measured_at=timestamp, value=3),
        ],
    )])
    metrics_server.envelopes = [
        envelope(suffix="v1Metric", payload=metric.SerializeToString()),
        envelope(suffix="v2Metrics", payload=batch.SerializeToString()),
    ]
    run_metrics(configuration, http_server, metrics_server)
    assert metrics_server.received == [
        {"type": "authorization", "bearer": "node-scoped-edge-token", "headers": {}},
        {"type": "subscribe", "subject": f"metrics.oursky.{configuration.lineage_id}.>"},
    ]
    (token_request,) = http_server.requests
    assert token_request.path == "/v1/edge-token"
    assert token_request.query["lineageId"] == [str(configuration.lineage_id)]
    assert token_request.headers["Authorization"] == "Bearer test-token"
    output = capsys.readouterr().out
    assert "1970-01-01T00:00:10.000000123Z camera.temperature=12.5 GAUGE ['source:node_controller']" in output
    assert "camera.frames=2.0 COUNT ['camera:astro']" in output
    assert "camera.frames=3.0 COUNT ['camera:astro']" in output
    assert "Finished listening for metrics" in output


def test_rejected_authorization_never_subscribes(configuration, http_server, metrics_server):
    metrics_server.authorized = False
    metrics_server.send_welcome = True
    with pytest.raises(RuntimeError, match="authorization failed"):
        run_metrics(configuration, http_server, metrics_server)
    assert len(metrics_server.received) == 1


def test_early_close_fails_instead_of_reporting_completion(configuration, http_server, metrics_server):
    metrics_server.close_after_subscription = True
    with pytest.raises(ConnectionClosed):
        run_metrics(configuration, http_server, metrics_server)


def test_invalid_protobuf_fails(configuration, http_server, metrics_server):
    metrics_server.envelopes = [envelope(suffix="v1Metric", payload=b"\xff")]
    with pytest.raises(DecodeError):
        run_metrics(configuration, http_server, metrics_server)
