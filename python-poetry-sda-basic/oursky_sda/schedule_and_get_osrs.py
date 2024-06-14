import datetime
import os
import ourskyai_sda_api
from pprint import pprint
from dotenv import load_dotenv
import typing
from datetime import timedelta

load_dotenv()  # take environment variables from .env.


# Configure Bearer authorization: BearerToken
configuration = ourskyai_sda_api.Configuration(
    access_token = os.environ["OURSKY_API_TOKEN"],
)

# Enter a context with an instance of the API client
with ourskyai_sda_api.ApiClient(configuration) as api_client:
    api_instance = ourskyai_sda_api.DefaultApi(api_client)

    # Get the ISS target
    targets = api_instance.v1_get_satellite_targets(norad_id="25544").targets
    pprint(targets)

    iss = typing.cast(ourskyai_sda_api.V1SatelliteTarget, targets[0])

    # fetch the upcoming potential observation windows for the iss
    # Add two days to the current timestamp
    new_timestamp = datetime.datetime.now() + timedelta(days=2)
    upcoming_passes = api_instance.v1_get_satellite_potentials(satellite_target_id=iss.id, until=new_timestamp)
    pprint(upcoming_passes)

    # add a task request to observe the iss
    try:
        request = ourskyai_sda_api.V1CreateOrganizationTargetRequest(
            satelliteTargetId= targets[0].id,
        )
        response = api_instance.v1_create_organization_target(request)
    except:
        print("Failed to create organization target")

    # fetch the OSRs for the organization
    osrs = api_instance.v1_get_observation_sequence_results(target_id=iss.id) 
    for osr in osrs:
        print(osr)
        for image_set in osr.image_sets:
            api_instance.v1_get_node_properties(image_set.node_id)