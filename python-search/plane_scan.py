"""Create a plane scan for a satellite target.

A plane scan searches along the target's orbital plane (in-track direction)
around the predicted position. This is the right first choice when the
uncertainty is dominated by along-track error.
"""

import argparse
import json
import os
import sys

import ourskyai_sda_api

ANOMALY_DEGREES = 2.0


def main():
    parser = argparse.ArgumentParser(description="Create a plane scan.")
    parser.add_argument("--target-id", required=True, help="UUID of the satellite target to search for.")
    parser.add_argument(
        "--plus-minus-anomaly-degrees",
        type=float,
        default=ANOMALY_DEGREES,
        help="Degrees on either side of the target's predicted anomaly to search.",
    )
    args = parser.parse_args()

    token = os.environ.get("OURSKY_API_TOKEN")
    if not token:
        sys.exit("OURSKY_API_TOKEN env var is not set. Create one at https://console.prod.oursky.ai/ -> Account -> General.")

    configuration = ourskyai_sda_api.Configuration(access_token=token)
    with ourskyai_sda_api.ApiClient(configuration) as api_client:
        api = ourskyai_sda_api.DefaultApi(api_client)
        request = ourskyai_sda_api.V1PlaneScanRequest(
            target_id=args.target_id,
            plus_minus_anomaly_angle_degrees=args.plus_minus_anomaly_degrees,
        )
        result = api.v2_create_plane_scan(request)

    print(json.dumps(result.to_dict(), indent=2, default=str))


if __name__ == "__main__":
    main()
