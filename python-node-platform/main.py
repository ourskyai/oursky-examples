import argparse

from dotenv import load_dotenv

from c2_capture_example import C2CaptureExample
from continuous_capture_example import ContinuousCaptureExample
from example_clients import ExampleClients
from example_configuration import ExampleConfiguration
from metrics_stream_example import MetricsStreamExample


def main() -> None:
    parser = argparse.ArgumentParser(description="OurSky node-platform examples")
    parser.add_argument("command", choices=["status", "c2-capture", "continuous-capture", "metrics"])
    arguments = parser.parse_args()
    load_dotenv()
    configuration = ExampleConfiguration()
    with ExampleClients(configuration=configuration) as clients:
        match arguments.command:
            case "status":
                status = clients.node_platform.v1_get_system_status(
                    lineage_id=configuration.lineage_id,
                    _request_timeout=configuration.timeout_seconds,
                )
                print(f"System status\n{status.model_dump_json(by_alias=True, indent=2)}")
                mount_model = clients.node_platform.v1_get_mount_model(
                    lineage_id=configuration.lineage_id,
                    _request_timeout=configuration.timeout_seconds,
                )
                print(f"Mount model\n{mount_model.model_dump_json(by_alias=True, indent=2)}")
            case "c2-capture":
                C2CaptureExample(
                    configuration=configuration, c2=clients.c2,
                ).run()
            case "continuous-capture":
                ContinuousCaptureExample(
                    configuration=configuration, node_platform=clients.node_platform,
                ).run()
            case "metrics":
                MetricsStreamExample(configuration=configuration, platform=clients.platform).run()


if __name__ == "__main__":
    main()
