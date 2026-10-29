import json
import os
from typing import Any

from dotenv import load_dotenv
import ourskyai_node_platform_api


def required_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise ValueError(f"{name} must be set")
    return value


def print_json(title: str, value: Any) -> None:
    print(f"\n{title}")
    print(json.dumps(value.to_dict(), indent=2, default=str))


def get_mount_model(api_instance, lineage_id: str) -> None:
    mount_model = api_instance.v1_get_mount_model(lineage_id=lineage_id)
    print_json("Mount model", mount_model)


def main() -> None:
    load_dotenv()

    configuration = ourskyai_node_platform_api.Configuration(
        access_token=required_env("OS_API_TOKEN"),
        host=required_env("OS_NODE_PLATFORM_BASE_URL").rstrip("/"),
    )

    with ourskyai_node_platform_api.ApiClient(configuration) as api_client:
        api_instance = ourskyai_node_platform_api.DefaultApi(api_client)
        lineage_id = required_env("OS_LINEAGE_ID")

        status = api_instance.v1_get_system_status(lineage_id=lineage_id)
        print_json("System status", status)

        get_mount_model(api_instance, lineage_id)

        # The examples below modify the current mount model. Uncomment the
        # calls you want to run.

        # point_request = ourskyai_node_platform_api.V1AddMountModelPointHereRequest(
        #     ra_degrees=123.45,
        #     dec_degrees=12.34,
        # )
        # api_instance.v1_add_mount_model_point_here(
        #     v1_add_mount_model_point_here_request=point_request,
        #     lineage_id=lineage_id,
        # )

        # api_instance.v1_add_mount_model_point_using_image(
        #     body="/path/to/image.fits",
        #     lineage_id=lineage_id,
        # )

        # api_instance.v1_add_mount_model_point_using_image(
        #     body="/path/to/image.fits",
        #     lineage_id=lineage_id,
        #     motor_a_encoder_position_degrees=10.5,
        #     motor_b_encoder_position_degrees=20.5,
        #     timestamp="2026-06-22T20:15:00Z",
        # )

        # api_instance.v1_remove_mount_model_point(
        #     index=0,
        #     lineage_id=lineage_id,
        # )

        # api_instance.v1_clear_mount_model(lineage_id=lineage_id)

        # get_mount_model(api_instance, lineage_id)


if __name__ == "__main__":
    try:
        main()
    except ourskyai_node_platform_api.ApiException as e:
        print(f"API error: {e}")
    except ValueError as e:
        print(f"Configuration error: {e}")
