# Source Extractor EE50 Computation Demo

Demonstrates spot-size measurement with the same SEP (Source Extractor) EE50 implementation used by `lib_image-proc`.

## Requirements

Install OpenCV:

```bash
# Ubuntu/Debian
sudo apt install libopencv-dev

# macOS
brew install opencv
```

SEP is vendored in `sep/` and built automatically.

## Build

```bash
cmake -S . -B build
cmake --build build --target compute_hfd_demo
```

## Run

From the project directory:

```bash
./build/compute_hfd_demo 69-focus-bottom-right-exposure-04.fits
```

Run without an argument to measure the generated Gaussian test image instead.

## What it does

- Loads an uncompressed 16-bit primary FITS image or an OpenCV-supported single-channel image
- Generates a simulated Gaussian star when no path is supplied
- Runs SEP background estimation and 5σ source extraction on the full frame
- Selects the brightest valid source and computes aperture photometry in 0.5-pixel steps
- Reports the EE50 diameter, centroid, peak, sky level, background RMS, total flux, and source count

## Result for `69-focus-bottom-right-exposure-04.fits`

```
Source Extractor EE50 Computation Demo

Input:  69-focus-bottom-right-exposure-04.fits
Image size:  1000x1000 px

Results:
  Sources:     1
  Sky level:   493.38 ADU
  Background:  3.79 ADU RMS
  Centroid:    (556.81, 576.78)
  Peak:        19573.32 ADU
  Total flux:  98750.94 ADU
  EE50:        2.657 px
```
