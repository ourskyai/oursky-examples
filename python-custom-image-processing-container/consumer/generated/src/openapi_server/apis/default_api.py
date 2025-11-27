# coding: utf-8

from typing import Dict, List  # noqa: F401
import importlib
import pkgutil

from openapi_server.apis.default_api_base import BaseDefaultApi
import openapi_server.impl

from fastapi import (  # noqa: F401
    APIRouter,
    Body,
    Cookie,
    Depends,
    Form,
    Header,
    HTTPException,
    Path,
    Query,
    Response,
    Security,
    status,
)

from openapi_server.models.extra_models import TokenModel  # noqa: F401
from openapi_server.models.error import Error
from openapi_server.models.process_image200_response import ProcessImage200Response
from openapi_server.models.process_image_request import ProcessImageRequest


router = APIRouter()

ns_pkg = openapi_server.impl
for _, name, _ in pkgutil.iter_modules(ns_pkg.__path__, ns_pkg.__name__ + "."):
    importlib.import_module(name)


@router.post(
    "/custom-image-processing/v1/images",
    responses={
        200: {"model": ProcessImage200Response, "description": "Image processed successfully"},
        400: {"model": Error, "description": "Invalid request payload. JSON schema validation failed."},
        422: {"model": Error, "description": "Invalid image data. Image could not be processed."},
        500: {"model": Error, "description": "Internal server error"},
    },
    tags=["default"],
    summary="Process new image",
    response_model_by_alias=True,
)
async def process_image(
    process_image_request: ProcessImageRequest = Body(None, description=""),
) -> ProcessImage200Response:
    """Called when a new image is available for processing. The primary image processing worker waits for the response to determine if the image was processed and where it was saved."""
    if not BaseDefaultApi.subclasses:
        raise HTTPException(status_code=500, detail="Not implemented")
    return await BaseDefaultApi.subclasses[0]().process_image(process_image_request)
