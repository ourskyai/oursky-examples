import math
import os
from collections.abc import Mapping
from pathlib import Path
from urllib.parse import urlsplit
from uuid import UUID


class ExampleConfiguration:
    def __init__(self, *, environment: Mapping[str, str] | None = None) -> None:
        self.environment = os.environ if environment is None else environment
        self.api_token = self.required("OS_API_TOKEN")
        self.lineage_id = UUID(self.required("OS_LINEAGE_ID"))
        self.cloud_base_url = self.environment.get(
            "OS_API_BASE_URL",
            self.environment.get("OS_NODE_PLATFORM_BASE_URL", "https://api.prod.oursky.ai"),
        ).strip().rstrip("/")
        parsed_base_url = urlsplit(self.cloud_base_url)
        if parsed_base_url.scheme not in {"http", "https"} or not parsed_base_url.hostname:
            raise ValueError(
                "OS_API_BASE_URL (or OS_NODE_PLATFORM_BASE_URL) must include "
                "http:// or https:// and a hostname"
            )
        self.output_directory = Path(self.environment.get("OS_OUTPUT_DIRECTORY", "output"))
        self.timeout_seconds = float(self.positive_integer("OS_TIMEOUT_SECONDS", 120))

    def required(self, name: str) -> str:
        value = self.environment.get(name, "").strip()
        if not value:
            raise ValueError(f"{name} must be set")
        return value

    def positive_integer(self, name: str, default: int) -> int:
        value = int(self.environment.get(name, str(default)))
        if value <= 0:
            raise ValueError(f"{name} must be positive")
        return value

    @property
    def optical_train_label(self) -> str:
        return self.required("OS_OPTICAL_TRAIN_LABEL")

    @property
    def camera_settings(self) -> dict[str, int | float]:
        exposure = float(self.environment.get("OS_EXPOSURE_SECONDS", "1.0"))
        if not math.isfinite(exposure) or exposure <= 0:
            raise ValueError("OS_EXPOSURE_SECONDS must be positive and finite")
        return {
            "exposure": exposure,
            "gain": int(self.environment.get("OS_GAIN", "0")),
            "binning": self.positive_integer("OS_BINNING", 1),
            "offset": int(self.environment.get("OS_OFFSET", "0")),
            "gain_mode": int(self.environment.get("OS_GAIN_MODE", "1")),
        }

    @property
    def receiver_host(self) -> str:
        return self.required("OS_STREAM_RECEIVER_HOST")

    @property
    def receiver_port(self) -> int:
        return int(self.environment.get("OS_STREAM_RECEIVER_PORT", "5000"))

    @property
    def frame_count(self) -> int:
        return self.positive_integer("OS_FRAME_COUNT", 5)

    @property
    def metrics_websocket_url(self) -> str:
        return self.required("OS_METRICS_WEBSOCKET_URL")

    @property
    def metrics_seconds(self) -> int:
        return self.positive_integer("OS_METRICS_SECONDS", 30)
