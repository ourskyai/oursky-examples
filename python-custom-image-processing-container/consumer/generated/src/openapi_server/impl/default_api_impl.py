from openapi_server.apis.default_api_base import BaseDefaultApi
from openapi_server.models.process_image200_response import ProcessImage200Response
from openapi_server.models.process_image_request import ProcessImageRequest
from openapi_server.models.health_check200_response import HealthCheck200Response

from astropy.io import fits
from astropy.io.fits import CompImageHDU
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
                if len(hdul) == 0:
                    raise RuntimeError("FITS file contains no HDUs")

                # Find compressed image HDUs (similar to Kotlin CompressedImageHDU handling)
                compressed_hdus = [hdu for hdu in hdul if isinstance(hdu, CompImageHDU)]

                if len(compressed_hdus) > 1:
                    raise RuntimeError("Unable to process image with more than one compressed Image HDU")

                # Use compressed HDU if available, otherwise find first HDU with data
                if compressed_hdus:
                    image_hdu = compressed_hdus[0]
                    image_data = image_hdu.data
                    header = image_hdu.header
                    is_compressed = True
                    print("Found compressed image HDU")
                else:
                    # Find the HDU with image data (primary HDU may be empty)
                    image_data = None
                    header = None
                    for hdu in hdul:
                        if hdu.data is not None:
                            image_data = hdu.data
                            header = hdu.header
                            break
                    is_compressed = False

                if image_data is None:
                    raise RuntimeError("No image data found in FITS file")

                # Blackout box algorithm: zero out the center 10% of the image
                print(f"Opened FITS image (compressed: {is_compressed})")

                processed_data = image_data.copy()

                # Get image dimensions (handle both 2D and multi-dimensional arrays)
                if processed_data.ndim >= 2:
                    height, width = processed_data.shape[-2:]

                    # Calculate center 10% region bounds
                    box_height = int(height * 0.1)
                    box_width = int(width * 0.1)

                    y_start = (height - box_height) // 2
                    y_end = y_start + box_height
                    x_start = (width - box_width) // 2
                    x_end = x_start + box_width

                    # Zero out the center region
                    if processed_data.ndim == 2:
                        processed_data[y_start:y_end, x_start:x_end] = 0
                    else:
                        # For multi-dimensional data, apply to all slices
                        processed_data[..., y_start:y_end, x_start:x_end] = 0

                    print(f"Applied blackout box: {box_width}x{box_height} pixels at center")

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

    async def health_check(self):
        return HealthCheck200Response(status="OK")
