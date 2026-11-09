import subprocess
import sys

import pytest

from example_configuration import ExampleConfiguration
import main


def test_command_is_required_before_reading_configuration():
    result = subprocess.run([sys.executable, "main.py"], capture_output=True, text=True)
    assert result.returncode == 2
    assert "the following arguments are required: command" in result.stderr
    assert "OS_API_TOKEN" not in result.stderr


def test_help_lists_all_commands():
    result = subprocess.run([sys.executable, "main.py", "--help"], capture_output=True, text=True)
    assert result.returncode == 0
    for command in ["status", "c2-capture", "continuous-capture", "metrics"]:
        assert command in result.stdout


def test_status_retains_existing_read_only_example(configuration, http_server, monkeypatch, capsys):
    http_server.responses.extend([{}, {"createdAt": "2026-10-29T00:00:00Z", "points": []}])
    for name in ["OS_API_TOKEN", "OS_LINEAGE_ID", "OS_API_BASE_URL"]:
        monkeypatch.setenv(name, configuration.environment[name])
    monkeypatch.setattr(sys, "argv", ["main.py", "status"])
    monkeypatch.setattr(main, "load_dotenv", lambda: None)
    main.main()
    assert [request.path for request in http_server.requests] == [
        "/node-platform/v1/system/status", "/node-platform/v1/mount/model",
    ]
    output = capsys.readouterr().out
    assert "System status" in output
    assert '"points": []' in output
    for request in http_server.requests:
        assert request.headers["Authorization"] == "Bearer test-token"
        assert request.query["lineageId"] == [str(configuration.lineage_id)]


def test_kotlin_defaults_and_legacy_base_url():
    configuration = ExampleConfiguration(environment={
        "OS_API_TOKEN": "test-token",
        "OS_LINEAGE_ID": "00000000-0000-0000-0000-000000000001",
        "OS_NODE_PLATFORM_BASE_URL": "http://localhost:8080/",
    })
    assert configuration.cloud_base_url == "http://localhost:8080"
    assert configuration.frame_count == 5
    assert configuration.metrics_seconds == 30
    assert configuration.timeout_seconds == 120
    assert configuration.receiver_port == 5000
    assert configuration.camera_settings == {
        "exposure": 1.0, "gain": 0, "binning": 1, "offset": 0, "gain_mode": 1,
    }


@pytest.mark.parametrize("name", ["OS_API_BASE_URL", "OS_NODE_PLATFORM_BASE_URL"])
def test_api_base_url_requires_http_scheme(name):
    with pytest.raises(ValueError, match="must include http:// or https:// and a hostname"):
        ExampleConfiguration(environment={
            "OS_API_TOKEN": "test-token",
            "OS_LINEAGE_ID": "00000000-0000-0000-0000-000000000001",
            name: "127.0.0.1:9080",
        })


@pytest.mark.parametrize("exposure", ["0", "-1", "nan", "inf"])
def test_invalid_exposure_fails_before_capture(configuration, exposure):
    configuration.environment["OS_EXPOSURE_SECONDS"] = exposure
    with pytest.raises(ValueError, match="OS_EXPOSURE_SECONDS"):
        _ = configuration.camera_settings
