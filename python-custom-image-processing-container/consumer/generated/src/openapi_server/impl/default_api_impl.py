from openapi_server.apis.default_api_base import BaseDefaultApi
from openapi_server.models.process_image200_response import ProcessImage200Response
from openapi_server.models.process_image_request import ProcessImageRequest

from astropy.io import fits
import os


class DefaultApiImpl(BaseDefaultApi):
    async def process_image(
        self,
        process_image_request: ProcessImageRequest,
    ) -> ProcessImage200Response:

        # log start of request
        print("Request:", process_image_request)

        # open FITS image
        try:
            with fits.open(process_image_request.raw_image_path) as hdul:
                # Get the primary image data
                image_data = hdul[0].data
                header = hdul[0].header

                # Do stuff with the image data here
                # image_data is a numpy array
                # For example: processed_data = some_processing_function(image_data)
                print("Opened FITS image")

                processed_data = image_data  # Placeholder - no processing yet

        except FileNotFoundError:
            raise RuntimeError(f"FITS file not found: {process_image_request.raw_image_path}")
        except Exception as e:
            raise RuntimeError(f"Failed to open or read FITS image: {e}")

        # save processed image
        processed_dir = process_image_request.processed_image_output_dir
        processed_image_path = None
        if processed_dir is not None:
            try:
                processed_image_path = os.path.join(
                    processed_dir,
                    os.path.basename(process_image_request.raw_image_path)
                )

                # Write the processed FITS file
                fits.writeto(processed_image_path, processed_data, header, overwrite=True)

            except Exception as e:
                raise RuntimeError(f"Failed to save processed FITS image: {e}")

        # return the response
        return ProcessImage200Response(
            processed_image_path=processed_image_path
        )