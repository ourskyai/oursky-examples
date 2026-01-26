# coding: utf-8

from typing import ClassVar, Dict, List, Tuple  # noqa: F401

from openapi_server.models.error import Error
from openapi_server.models.health_check200_response import HealthCheck200Response
from openapi_server.models.process_image200_response import ProcessImage200Response
from openapi_server.models.process_image_request import ProcessImageRequest


class BaseDefaultApi:
    subclasses: ClassVar[Tuple] = ()

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        BaseDefaultApi.subclasses = BaseDefaultApi.subclasses + (cls,)
    async def process_image(
        self,
        process_image_request: ProcessImageRequest,
    ) -> ProcessImage200Response:
        """Called when a new image is available for processing. The primary image processing worker waits for the response to determine if the image was processed and where it was saved."""
        ...


    async def health_check(
        self,
    ) -> HealthCheck200Response:
        """Simple health check to verify that the custom image processing container is running."""
        ...
