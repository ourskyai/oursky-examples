from contextlib import ExitStack

import ourskyai_c2_api as c2
import ourskyai_node_platform_api as node_platform
import ourskyai_platform_api as platform

from example_configuration import ExampleConfiguration


class ExampleClients(ExitStack):
    def __init__(self, *, configuration: ExampleConfiguration) -> None:
        super().__init__()
        c2_client = self.enter_context(c2.ApiClient(
            configuration=c2.Configuration(
                access_token=configuration.api_token, host=configuration.cloud_base_url,
            ),
        ))
        node_platform_client = self.enter_context(node_platform.ApiClient(
            configuration=node_platform.Configuration(
                access_token=configuration.api_token, host=configuration.cloud_base_url,
            ),
        ))
        platform_client = self.enter_context(platform.ApiClient(
            configuration=platform.Configuration(
                access_token=configuration.api_token, host=configuration.cloud_base_url,
            ),
        ))
        self.c2 = c2.DefaultApi(api_client=c2_client)
        self.node_platform = node_platform.DefaultApi(api_client=node_platform_client)
        self.platform = platform.DefaultApi(api_client=platform_client)
