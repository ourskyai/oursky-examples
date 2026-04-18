"""Create a RIC volumetric search for a satellite target.

A RIC volumetric search covers a 3D box in the target's Radial / In-track /
Cross-track frame around the predicted position. Use it when uncertainty is
non-trivial in more than just the along-track direction.
"""

import argparse
import json
import os
import sys

import ourskyai_sda_api

RADIAL_3SIGMA_METERS = 500.0
INTRACK_3SIGMA_METERS = 10_000.0
CROSSTRACK_3SIGMA_METERS = 1_000.0


def main():
    parser = argparse.ArgumentParser(description="Create a RIC volumetric search.")
    parser.add_argument("--target-id", required=True, help="UUID of the satellite target to search for.")
    args = parser.parse_args()

    token = os.environ.get("OURSKY_API_TOKEN")
    if not token:
        sys.exit("OURSKY_API_TOKEN env var is not set. Create one at https://console.prod.oursky.ai/ -> Account -> General.")

    configuration = ourskyai_sda_api.Configuration(access_token=token)
    with ourskyai_sda_api.ApiClient(configuration) as api_client:
        api = ourskyai_sda_api.DefaultApi(api_client)
        request = ourskyai_sda_api.V1RicVolumeSearchRequest(
            target_id=args.target_id,
            radial3_sigma_meters=RADIAL_3SIGMA_METERS,
            intrack3_sigma_meters=INTRACK_3SIGMA_METERS,
            crosstrack3_sigma_meters=CROSSTRACK_3SIGMA_METERS,
        )
        result = api.v2_create_ric_volume_search(request)

    print(json.dumps(result.to_dict(), indent=2, default=str))


if __name__ == "__main__":
    main()
