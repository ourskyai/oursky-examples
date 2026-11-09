import base64
import json
import time

from ourskyai_platform_api import DefaultApi as PlatformApi
from websockets.sync.client import connect

from example_configuration import ExampleConfiguration
from generated.metrics_pb2 import MetricType, V1Metric, V2MetricBatch


class MetricsStreamExample:
    def __init__(self, *, configuration: ExampleConfiguration, platform: PlatformApi) -> None:
        self.configuration = configuration
        self.platform = platform

    def run(self) -> None:
        configuration = self.configuration
        websocket_url = configuration.metrics_websocket_url
        duration = configuration.metrics_seconds
        edge_token = self.platform.v1_generate_edge_token(
            lineage_id=configuration.lineage_id,
            _request_timeout=configuration.timeout_seconds,
        ).token
        with connect(
            websocket_url, open_timeout=configuration.timeout_seconds,
            close_timeout=5, proxy=None,
        ) as websocket:
            websocket.send(json.dumps({"type": "authorization", "bearer": edge_token, "headers": {}}))
            authorization_deadline = time.monotonic() + 15
            while True:
                remaining = authorization_deadline - time.monotonic()
                if remaining <= 0:
                    raise TimeoutError("Timed out waiting for EC WebSocket authorization")
                authorization = json.loads(websocket.recv(timeout=remaining))
                if authorization.get("type") == "authorization":
                    break
            if not authorization.get("authorized"):
                raise RuntimeError("EC WebSocket authorization failed")
            subject = f"metrics.oursky.{configuration.lineage_id}.>"
            websocket.send(json.dumps({"type": "subscribe", "subject": subject}))
            print(f"Subscribed to {subject}", flush=True)
            deadline = time.monotonic() + duration
            while (remaining := deadline - time.monotonic()) > 0:
                try:
                    message = json.loads(websocket.recv(timeout=remaining))
                except TimeoutError:
                    break
                if message.get("type") == "nats":
                    self._print_metrics(
                        subject=message["subject"],
                        payload=base64.b64decode(message["data"], validate=True),
                    )
        print("Finished listening for metrics")

    def _print_metrics(self, *, subject: str, payload: bytes) -> None:
        match subject.rsplit(".", 1)[-1]:
            case "v1Metric":
                metric = V1Metric.FromString(payload)
                print(
                    f"{metric.measured_at.ToJsonString()} {metric.name}={metric.value} "
                    f"{MetricType.Name(metric.type)} {list(metric.tags)}",
                    flush=True,
                )
            case "v2Metrics":
                for series in V2MetricBatch.FromString(payload).series:
                    for point in series.points:
                        print(
                            f"{point.measured_at.ToJsonString()} {series.name}={point.value} "
                            f"{MetricType.Name(series.type)} {list(series.tags)}",
                            flush=True,
                        )
