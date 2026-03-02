#include "image_processor.h"
#include "snpe_worker.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <fitsio.h>

namespace fs = std::filesystem;

struct FitsImage {
    std::vector<float> data; // HWC interleaved, normalized [0,1]
    int height;
    int width;
    int channels;
};

// RAII wrapper for cfitsio file handle
struct FitsFile {
    fitsfile* fptr = nullptr;
    int status = 0;

    explicit FitsFile(const std::string& path) {
        fits_open_file(&fptr, path.c_str(), READONLY, &status);
        if (status) {
            char msg[80];
            fits_get_errstatus(status, msg);
            throw std::runtime_error("Cannot open FITS file: " + std::string(msg));
        }
    }

    ~FitsFile() {
        if (fptr) {
            int s = 0;
            fits_close_file(fptr, &s);
        }
    }

    FitsFile(const FitsFile&) = delete;
    FitsFile& operator=(const FitsFile&) = delete;
};

static FitsImage read_fits_image(const std::string& path) {
    FitsFile fits(path);

    int naxis = 0;
    fits_get_img_dim(fits.fptr, &naxis, &fits.status);
    if (fits.status || naxis < 2) {
        throw std::runtime_error(
            "FITS image must be at least 2D (got " + std::to_string(naxis) + "D)");
    }

    long naxes[3] = {1, 1, 1};
    fits_get_img_size(fits.fptr, std::min(naxis, 3), naxes, &fits.status);
    if (fits.status) {
        char msg[80];
        fits_get_errstatus(fits.status, msg);
        throw std::runtime_error("Failed to read FITS dimensions: " + std::string(msg));
    }

    long width  = naxes[0]; // NAXIS1
    long height = naxes[1]; // NAXIS2
    long depth  = (naxis >= 3) ? naxes[2] : 1; // NAXIS3 or 1

    // Read all pixels as float (cfitsio converts any BITPIX automatically)
    long nelements = width * height * depth;
    std::vector<float> pixels(nelements);
    long fpixel[3] = {1, 1, 1}; // FITS is 1-indexed
    fits_read_pix(fits.fptr, TFLOAT, fpixel, nelements,
                  nullptr, pixels.data(), nullptr, &fits.status);
    if (fits.status) {
        char msg[80];
        fits_get_errstatus(fits.status, msg);
        throw std::runtime_error("Failed to read FITS pixels: " + std::string(msg));
    }

    // Normalize to [0, 1]
    float min_val = *std::min_element(pixels.begin(), pixels.end());
    float max_val = *std::max_element(pixels.begin(), pixels.end());
    float range = max_val - min_val;
    if (range > 0) {
        for (auto& p : pixels) {
            p = (p - min_val) / range;
        }
    }

    int channels = static_cast<int>(depth);

    if (channels == 1) {
        // Grayscale: replicate to 3 channels for the RGB model
        std::vector<float> rgb(height * width * 3);
        for (long i = 0; i < height * width; ++i) {
            rgb[i * 3 + 0] = pixels[i];
            rgb[i * 3 + 1] = pixels[i];
            rgb[i * 3 + 2] = pixels[i];
        }
        return {std::move(rgb), static_cast<int>(height), static_cast<int>(width), 3};
    }

    // Multi-channel: FITS stores planes sequentially [plane0][plane1][plane2]
    // Convert to interleaved HWC layout
    std::vector<float> hwc(height * width * channels);
    for (long y = 0; y < height; ++y) {
        for (long x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                hwc[(y * width + x) * channels + c] =
                    pixels[c * height * width + y * width + x];
            }
        }
    }
    return {std::move(hwc), static_cast<int>(height), static_cast<int>(width), channels};
}

// Bilinear interpolation resize for HWC float data
static std::vector<float> bilinear_resize(
        const float* src, int src_h, int src_w, int channels,
        int dst_h, int dst_w) {
    std::vector<float> dst(dst_h * dst_w * channels);

    for (int y = 0; y < dst_h; ++y) {
        float src_y = (y + 0.5f) * src_h / dst_h - 0.5f;
        int y0 = std::max(0, static_cast<int>(std::floor(src_y)));
        int y1 = std::min(src_h - 1, y0 + 1);
        float fy = src_y - static_cast<float>(y0);

        for (int x = 0; x < dst_w; ++x) {
            float src_x = (x + 0.5f) * src_w / dst_w - 0.5f;
            int x0 = std::max(0, static_cast<int>(std::floor(src_x)));
            int x1 = std::min(src_w - 1, x0 + 1);
            float fx = src_x - static_cast<float>(x0);

            for (int c = 0; c < channels; ++c) {
                float v00 = src[(y0 * src_w + x0) * channels + c];
                float v01 = src[(y0 * src_w + x1) * channels + c];
                float v10 = src[(y1 * src_w + x0) * channels + c];
                float v11 = src[(y1 * src_w + x1) * channels + c];

                dst[(y * dst_w + x) * channels + c] =
                    (1 - fy) * ((1 - fx) * v00 + fx * v01) +
                    fy * ((1 - fx) * v10 + fx * v11);
            }
        }
    }
    return dst;
}

static nlohmann::json classifications_to_json(const std::vector<Classification>& results) {
    nlohmann::json j = nlohmann::json::array();
    for (auto& c : results) {
        j.push_back({{"index", c.index},
                     {"confidence", c.confidence},
                     {"label", c.label}});
    }
    return j;
}

ProcessResult process_image(SnpeWorker& worker,
                            const std::string& image_path,
                            double /*timeout_seconds*/) {
    if (!fs::exists(image_path)) {
        return {false, "Image file does not exist: " + image_path, {}};
    }

    try {
        auto fits = read_fits_image(image_path);

        // Resize to model's expected dimensions if needed
        std::vector<float> image_data;
        int target_h = static_cast<int>(worker.input_height());
        int target_w = static_cast<int>(worker.input_width());

        if (fits.height == target_h && fits.width == target_w &&
            fits.channels == static_cast<int>(worker.input_channels())) {
            image_data = std::move(fits.data);
        } else {
            image_data = bilinear_resize(
                fits.data.data(), fits.height, fits.width, fits.channels,
                target_h, target_w);
        }

        auto results = worker.infer(image_data);
        auto json_results = classifications_to_json(results);

        return {true, "", json_results};
    } catch (const std::exception& e) {
        return {false, e.what(), {}};
    }
}
