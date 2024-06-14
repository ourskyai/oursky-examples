import datetime
import os
import ourskyai_sda_api
from pprint import pprint
from dotenv import load_dotenv
import typing
from typing import List  
from datetime import timedelta, date
from json import dumps

load_dotenv()  # take environment variables from .env.

# Configure Bearer authorization: BearerToken
configuration = ourskyai_sda_api.Configuration(
    access_token = os.environ["OURSKY_API_TOKEN"],
)

def json_serial(obj):
    """JSON serializer for objects not serializable by default json code"""

    if isinstance(obj, (datetime.datetime, datetime.date)):
        return obj.isoformat()
    raise TypeError ("Type %s not serializable" % type(obj))

norad_ids = ["25875"]  # List[str] | A list of NORAD IDs of the satellite targets to fetch.
write_to_disk = True # bool | If true, the response will be written to a file. (optional) (default to False)
from_time = datetime.datetime.now(datetime.timezone.utc) - timedelta(days=1)  # datetime | The start time of the observation sequence results to fetch. (optional)

# create osrs directory if it does not exist
if write_to_disk:
    if not os.path.exists("osrs"):
        os.makedirs("osrs")

# Enter a context with an instance of the API client
with ourskyai_sda_api.ApiClient(configuration) as api_client:
    api_instance = ourskyai_sda_api.DefaultApi(api_client)

    for norad_id in norad_ids:
        targets = api_instance.v1_get_satellite_targets(norad_id=norad_id).targets

        target = typing.cast(ourskyai_sda_api.V1SatelliteTarget, targets[0])

        print(f"Fetching OSRs for ({target.norad_id})")

        # fetch the OSRs for the organization
        osrs: List[ourskyai_sda_api.V1ObservationSequenceResult] = []
        more = True
        after_time = None
        while more:
            batch = typing.cast(typing.List[ourskyai_sda_api.V1ObservationSequenceResult], api_instance.v1_get_observation_sequence_results(target_id=target.id, after=after_time))
            filtered = [osr for osr in batch if from_time <= osr.created_at]
            print(f"Fetched {len(filtered)} additional OSRs")
            osrs.extend(filtered)
            if len(filtered) != 5:
                more = False
            else:
                after_time = batch[-1].created_at

        print(f"Fetched {len(osrs)} total OSRs for ({target.norad_id})")
        for osr in osrs:
            if write_to_disk:
                with open(f"osrs/{norad_id}_{osr.created_at}.json", "w") as f:
                    f.write(dumps(osr.to_dict(), default=json_serial, indent=4, sort_keys=True))
            else:
                print(osr.to_str())
