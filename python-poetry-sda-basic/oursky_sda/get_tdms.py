import datetime
import os
import ourskyai_sda_api
from pprint import pprint
from dotenv import load_dotenv

load_dotenv()  # take environment variables from .env.


# Configure Bearer authorization: BearerToken
configuration = ourskyai_sda_api.Configuration(
    access_token = os.environ["OURSKY_API_TOKEN"],
)

# Enter a context with an instance of the API client
with ourskyai_sda_api.ApiClient(configuration) as api_client:
    api_instance = ourskyai_sda_api.DefaultApi(api_client)
    pprint(api_instance.v1_get_tdms(after=datetime.datetime(2023, 12, 29)))