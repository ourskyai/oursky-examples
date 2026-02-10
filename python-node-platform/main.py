import json
import os

from dotenv import load_dotenv
import ourskyai_node_platform_api

load_dotenv()

configuration = ourskyai_node_platform_api.Configuration(
    access_token=os.environ["OS_API_TOKEN"],
    host=os.environ["OS_NODE_PLATFORM_BASE_URL"].rstrip("/")
)

with ourskyai_node_platform_api.ApiClient(configuration) as api_client:
    api_instance = ourskyai_node_platform_api.DefaultApi(api_client)
    lineage_id = os.environ["OS_LINEAGE_ID"]

    try:
        status = api_instance.v1_get_system_status(lineage_id=lineage_id)
        print(json.dumps(status.to_dict(), indent=2, default=str))
    except ourskyai_node_platform_api.ApiException as e:
        print(f"API error: {e}")
