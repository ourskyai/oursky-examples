from openapi_server.apis.default_api_base import BaseDefaultApi
from openapi_server.models.process_image200_response import ProcessImage200Response
from openapi_server.models.process_image_request import ProcessImageRequest
from openapi_server.models.health_check200_response import HealthCheck200Response

from astropy.io import fits
from astropy.io.fits import CompImageHDU
import numpy as np
import os


def access_fits_image_data(file_path: str) -> tuple[np.ndarray, fits.Header]:
    """
    Open a FITS file and extract the image data and header.

    This helper demonstrates how to access image data from FITS files,
    handling both compressed and uncompressed formats.

    Args:
        file_path: Path to the FITS file

    Returns:
        A tuple of (image_data, header) where image_data is a numpy array

    Raises:
        FileNotFoundError: If the FITS file doesn't exist
        RuntimeError: If the file has no HDUs, multiple compressed HDUs, or no image data
    """
    with fits.open(file_path) as hdul:
        if len(hdul) == 0:
            raise RuntimeError("FITS file contains no HDUs")

        compressed_hdus = [hdu for hdu in hdul if isinstance(hdu, CompImageHDU)]

        if len(compressed_hdus) > 1:
            raise RuntimeError("Unable to process image with more than one compressed Image HDU")

        # Use compressed HDU if available, otherwise find first HDU with data
        if compressed_hdus:
            image_hdu = compressed_hdus[0]
            return image_hdu.data.copy(), image_hdu.header.copy()

        # Find the HDU with image data (primary HDU may be empty)
        for hdu in hdul:
            if hdu.data is not None:
                return hdu.data.copy(), hdu.header.copy()

        raise RuntimeError("No image data found in FITS file")


def process_image_data(image_data: np.ndarray, box_fraction: float = 0.1) -> np.ndarray:
    """
    Apply a blackout box to the center of an image.

    This helper demonstrates a simple image processing operation:
    zeroing out a rectangular region in the center of the image.

    Args:
        image_data: Input image as a numpy array (2D or higher dimensional)
        box_fraction: Fraction of image dimensions for the box size (default 0.1 = 10%)

    Returns:
        Processed image with the center region blacked out
    """
    processed_data = image_data.copy()

    # Get image dimensions (handle both 2D and multi-dimensional arrays)
    if processed_data.ndim >= 2:
        height, width = processed_data.shape[-2:]

        # Calculate center box region bounds
        box_height = int(height * box_fraction)
        box_width = int(width * box_fraction)

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

    return processed_data


def save_fits_image(
    image_data: np.ndarray,
    header: fits.Header,
    output_dir: str,
    filename: str,
) -> str:
    """
    Save processed image data to a FITS file.

    This helper demonstrates how to write image data back to a FITS file,
    preserving the original header metadata.

    Args:
        image_data: The processed image data as a numpy array
        header: The FITS header to include with the image
        output_dir: Directory where the output file will be saved
        filename: Name for the output file

    Returns:
        The full path to the saved file

    Raises:
        RuntimeError: If the file cannot be saved
    """
    output_path = os.path.join(output_dir, filename)
    fits.writeto(output_path, image_data, header, overwrite=True)
    print(f"Saved processed image to: {output_path}")
    return output_path


class DefaultApiImpl(BaseDefaultApi):
    async def process_image(
        self,
        process_image_request: ProcessImageRequest,
    ) -> ProcessImage200Response:

        print("Request:", process_image_request)

        try:
            # Step 1: Access the image data from the FITS file
            image_data, header = access_fits_image_data(process_image_request.raw_image_path)
            print("Opened FITS image")

            # Step 2: Process the image (apply blackout box to center 10%)
            processed_data = process_image_data(image_data, box_fraction=0.1)

            # Step 3: Save the processed image
            processed_image_path = None
            if process_image_request.processed_image_output_dir is not None:
                processed_image_path = save_fits_image(
                    processed_data,
                    header,
                    process_image_request.processed_image_output_dir,
                    os.path.basename(process_image_request.raw_image_path),
                )

        except FileNotFoundError:
            raise RuntimeError(f"FITS file not found: {process_image_request.raw_image_path}")
        except Exception as e:
            raise RuntimeError(f"Failed to process FITS image: {e}")

        # return the response
        return ProcessImage200Response(
            processed_image_path=processed_image_path
        )

    async def health_check(self):
        return HealthCheck200Response(status="OK")
