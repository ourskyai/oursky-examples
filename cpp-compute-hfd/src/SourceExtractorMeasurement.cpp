#include "os_image_proc/SourceExtractorMeasurement.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <opencv2/imgproc.hpp>

extern "C" {
#include "sep.h"
}

namespace {

constexpr float kDetectionKernel[9] = {1.0f, 2.0f, 1.0f, 2.0f, 4.0f, 2.0f, 1.0f, 2.0f, 1.0f}; // source extractor default
constexpr int64_t kDetectionKernelWidth = 3; // source extractor default
constexpr int64_t kDetectionKernelHeight = 3; // source extractor default
constexpr int kCleanEnabled = 1; // source extractor default
constexpr double kCleanParam = 1.0; // source extractor default
constexpr int kApertureSubpixExact = 0; // from unc ee50.py line 69

std::vector<double> apertureRadii() {
  const int count = static_cast<int>(std::lround(SourceExtractorMeasurement::kMaxApertureRadius /
                                                 SourceExtractorMeasurement::kApertureRadiusStep));
  std::vector<double> radii;
  radii.reserve(count);
  for (int i = 0; i < count; ++i) {
    radii.push_back(SourceExtractorMeasurement::kApertureRadiusStep * (i + 1));
  }
  return radii;
}

}  // namespace

std::optional<SourceExtractorResult> SourceExtractorMeasurement::measure(const cv::Mat& image) {
  if (image.empty()) {
    return std::nullopt;
  }

  // loadFitsFile flips FITS vertically on load; the SEP reference runs on the unflipped orientation.
  cv::Mat oriented;
  cv::flip(image, oriented, 0);

  cv::Mat img32;
  oriented.convertTo(img32, CV_32F);
  if (!img32.isContinuous()) {
    img32 = img32.clone();
  }
  const int64_t width = img32.cols;
  const int64_t height = img32.rows;
  const auto pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
  auto* data = reinterpret_cast<float*>(img32.data);

  sep_image backgroundInput{};
  backgroundInput.data = data;
  backgroundInput.dtype = SEP_TFLOAT;
  backgroundInput.w = width;
  backgroundInput.h = height;
  sep_bkg* background = nullptr;
  if (sep_background(&backgroundInput, kBackgroundTile, kBackgroundTile, kBackgroundFilterTiles,
                     kBackgroundFilterTiles, 0.0, &background) != 0 ||
      background == nullptr) {
    return std::nullopt;
  }
  const float backgroundRms = sep_bkg_globalrms(background);

  std::vector<float> detectionData(data, data + pixelCount);
  sep_bkg_subarray(background, detectionData.data(), SEP_TFLOAT);

  sep_image detectionInput{};
  detectionInput.data = detectionData.data();
  detectionInput.dtype = SEP_TFLOAT;
  detectionInput.w = width;
  detectionInput.h = height;
  detectionInput.noiseval = backgroundRms;
  detectionInput.noise_type = SEP_NOISE_STDDEV;

  sep_catalog* catalog = nullptr;
  const int extractStatus = sep_extract(&detectionInput, kDetectionThresholdSigma, SEP_THRESH_REL,
                                         kMinDetectionArea, kDetectionKernel, kDetectionKernelWidth,
                                         kDetectionKernelHeight, SEP_FILTER_MATCHED, kDeblendThresholds,
                                         kDeblendContrast, kCleanEnabled, kCleanParam, &catalog);
  if (extractStatus != 0 || catalog == nullptr) {
    sep_bkg_free(background);
    return std::nullopt;
  }

  const double peakFloor = std::max(kPeakFloorAbsolute, kPeakFloorRmsMultiple * backgroundRms);
  int brightest = -1;
  float brightestPeak = -1.0f;
  int sourceCount = 0;
  for (int i = 0; i < catalog->nobj; ++i) {
    const float peak = catalog->peak[i];
    if (peak >= peakFloor && peak <= kPeakCeiling) {
      ++sourceCount;
      if (peak > brightestPeak) {
        brightestPeak = peak;
        brightest = i;
      }
    }
  }
  if (brightest < 0) {
    sep_catalog_free(catalog);
    sep_bkg_free(background);
    return std::nullopt;
  }
  const double sourceX = catalog->x[brightest];
  const double sourceY = catalog->y[brightest];
  const float sourcePeak = catalog->peak[brightest];
  sep_catalog_free(catalog);
  sep_bkg_free(background);

  const double innerRadiusSquared = kAnnulusInnerRadius * kAnnulusInnerRadius;
  const double outerRadiusSquared = kAnnulusOuterRadius * kAnnulusOuterRadius;
  double annulusSum = 0.0;
  int64_t annulusCount = 0;
  for (int64_t row = 0; row < height; ++row) {
    const float* rowPtr = img32.ptr<float>(static_cast<int>(row));
    const double dySquared = (static_cast<double>(row) - sourceY) * (static_cast<double>(row) - sourceY);
    for (int64_t col = 0; col < width; ++col) {
      const double distanceSquared =
          (static_cast<double>(col) - sourceX) * (static_cast<double>(col) - sourceX) + dySquared;
      if (distanceSquared >= innerRadiusSquared && distanceSquared <= outerRadiusSquared) {
        annulusSum += rowPtr[col];
        ++annulusCount;
      }
    }
  }
  const double annulusMean = annulusCount > 0 ? annulusSum / static_cast<double>(annulusCount) : 0.0;

  cv::Mat skySubtracted = img32 - static_cast<float>(annulusMean);
  if (!skySubtracted.isContinuous()) {
    skySubtracted = skySubtracted.clone();
  }

  sep_image apertureInput{};
  apertureInput.data = skySubtracted.data;
  apertureInput.dtype = SEP_TFLOAT;
  apertureInput.w = width;
  apertureInput.h = height;

  const std::vector<double> radii = apertureRadii();
  std::vector<double> enclosed;
  enclosed.reserve(radii.size());
  for (const double radius : radii) {
    double sum = 0.0;
    double sumError = 0.0;
    double area = 0.0;
    short flag = 0;
    if (sep_sum_circle(&apertureInput, sourceX, sourceY, radius, 0, kApertureSubpixExact, 0, &sum,
                       &sumError, &area, &flag) != 0) {
      return std::nullopt;
    }
    enclosed.push_back(sum);
  }

  const double halfFlux = 0.5 * enclosed.back();
  int firstAboveHalf = 0;
  for (size_t i = 0; i < enclosed.size(); ++i) {
    if (enclosed[i] >= halfFlux) {
      firstAboveHalf = static_cast<int>(i);
      break;
    }
  }
  const double innerRadius = firstAboveHalf == 0 ? 0.0 : radii[firstAboveHalf - 1];
  const double innerFlux = firstAboveHalf == 0 ? 0.0 : enclosed[firstAboveHalf - 1];
  const double outerRadius = radii[firstAboveHalf];
  const double outerFlux = enclosed[firstAboveHalf];
  if (outerFlux <= innerFlux) {
    return std::nullopt;
  }
  const double fraction = (halfFlux - innerFlux) / (outerFlux - innerFlux);
  const double ee50Radius = innerRadius + fraction * (outerRadius - innerRadius);

  SourceExtractorResult result{};
  result.ee50Diameter = static_cast<float>(2.0 * ee50Radius);
  result.centroidX = static_cast<float>(sourceX);
  result.centroidY = static_cast<float>(static_cast<double>(height - 1) - sourceY);
  result.peak = sourcePeak;
  result.backgroundRms = backgroundRms;
  result.skyLevel = static_cast<float>(annulusMean);
  result.totalFlux = static_cast<float>(enclosed.back());
  result.sourceCount = sourceCount;
  return result;
}
