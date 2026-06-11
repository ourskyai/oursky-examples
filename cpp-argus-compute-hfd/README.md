# ARGUS HFD Demo

Self-contained demonstration of HFD (Half-Flux Diameter) computation for star images.

## Requirements

Install OpenCV:

```bash
# Ubuntu/Debian
sudo apt install libopencv-dev

# macOS
brew install opencv
```

## Build

```bash
mkdir build && cd build
cmake ..
make demo_argus
```

## Run

From the build directory:

```bash
./test/argus_demo/demo_argus
```

## What it does

- Generates a simulated Gaussian star (σ=2 pixels) and saves it to `/tmp/generated_star.jpg`
- Computes background, centroid, and HFD
- Compares result to theoretical and empirical values

## Example Output

```
HFD Computation Demo

Input:  sigma = 2 px

Generating simulated star:
  Image size:  100x100 px
  Center:      (50, 50)
  Peak:        50000 ADU
  Background:  1000 ADU
  Sigma:       2 px
  Saved:       /tmp/generated_star.jpg

Results:
  Background:  1000.0 ± 0.00 ADU
  Centroid:    (25.00, 25.00)
  HFD:         5.099 px

Expected:
  Theoretical: 4.710 px  (2.355σ)
  Empirical:   5.100 px  (2.550σ)
  Error:       -0.001 px (0.0%)
```

