// Self-contained HFD computation utilities
// Requires: OpenCV, C++ standard library

#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Background and centroid computation parameters
constexpr float  kBackgroundSigmaThreshold = 3.0f;   // Sigma threshold for centroid computation
constexpr int    kMinBackgroundRadius = 20;          // Minimum radius for background annulus (pixels)
constexpr int    kMaxBackgroundRadius = 25;          // Maximum radius for background annulus (pixels)

// HFD computation parameters
constexpr double kMaxApertureRadius = 10.0;          // Maximum aperture radius for HFD (pixels)
constexpr float  kHFDBackgroundMultiplier = 0.0f;    // Background stddev multiplier for HFD (0 = level only)

// Sigma clipping parameters (for robust background estimation)
constexpr double kClippingSigma = 3.0;               // Sigma clipping threshold
constexpr int    kClippingMaxIterations = 5;         // Maximum sigma clipping iterations

struct BackgroundStats {
  double level;
  double stddev;
  BackgroundStats(double l = 0.0, double s = 0.0) : level(l), stddev(s) {}
};

struct BackgroundAndCentroid {
  BackgroundStats background;
  cv::Point2f centroid;
};

// Compute mean and standard deviation of a vector
inline std::pair<double, double> calculateMeanAndStdDev(const std::vector<double>& data) {
  if (data.empty()) {
    return {0.0, 0.0};
  }

  double sum = std::accumulate(data.begin(), data.end(), 0.0);
  double mean = sum / data.size();

  double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0, std::plus<double>(),
                                     [mean](double a, double b) { return (a - mean) * (b - mean); });
  double stddev = std::sqrt(sq_sum / data.size());

  return {mean, stddev};
}

// Compute median of a vector (sorts a copy)
inline double computeMedian(const std::vector<double>& data) {
  if (data.empty()) {
    return 0.0;
  }

  std::vector<double> sortedData = data;
  std::sort(sortedData.begin(), sortedData.end());

  const size_t n = sortedData.size();
  if (n % 2 == 0) {
    // Even number of elements: average of two middle values
    return (sortedData[n / 2 - 1] + sortedData[n / 2]) / 2.0;
  } else {
    // Odd number of elements: middle value
    return sortedData[n / 2];
  }
}

// Perform sigma clipping on data around a center value
// Iteratively removes outliers beyond sigma * stddev from the center
inline std::vector<double> computeSigmaClipping(const std::vector<double>& data, double center, double sigma = 3.0,
                                                 int maxIterations = 5) {
  if (data.empty()) {
    return std::vector<double>();
  }

  std::vector<double> clippedData = data;

  for (int iteration = 0; iteration < maxIterations; ++iteration) {
    if (clippedData.empty()) {
      break;
    }

    // Compute standard deviation of current clipped data
    const auto [mean, stddev] = calculateMeanAndStdDev(clippedData);

    if (stddev <= 0.0) {
      break;  // No variation, no more clipping needed
    }

    // Define clipping bounds
    const double lowerBound = center - sigma * stddev;
    const double upperBound = center + sigma * stddev;

    // Filter data within bounds
    std::vector<double> nextClippedData;
    nextClippedData.reserve(clippedData.size());

    for (double value : clippedData) {
      if (value >= lowerBound && value <= upperBound) {
        nextClippedData.push_back(value);
      }
    }

    // If no values were removed, we're done
    if (nextClippedData.size() == clippedData.size()) {
      break;
    }

    clippedData = std::move(nextClippedData);
  }

  return clippedData;
}

// Helper: Compute weight for a pixel's contribution to star metrics
// Returns (pixelValue - backgroundLevel) if pixelValue > inclusionThreshold and weight > 0, else 0.0
// This is used consistently across centroid and FWHM calculations
inline double computeWeight(double pixelValue, double inclusionThreshold, double backgroundLevel) {
  // Only include pixels above threshold
  if (pixelValue <= inclusionThreshold) {
    return 0.0;
  }

  // Weight by (intensity - background), not (intensity - threshold)
  double weight = pixelValue - backgroundLevel;

  // Ensure weight is positive
  if (weight <= 0.0) {
    return 0.0;
  }

  return weight;
}

// Helper: Compute total flux and collect pixels within aperture
struct ApertureData {
  double totalFlux;
  std::vector<std::pair<double, float>> pixelsByDistance;  // (distance, flux)
};

inline ApertureData collectAperturePixels(const cv::Mat& floatMat, const cv::Point2f& centroid, double maxRadius) {
  ApertureData data;
  data.totalFlux = 0.0;

  for (int y = 0; y < floatMat.rows; y++) {
    for (int x = 0; x < floatMat.cols; x++) {
      float pixel = floatMat.at<float>(y, x);
      double dx = (x + 0.5) - centroid.x;
      double dy = (y + 0.5) - centroid.y;
      double dist = std::sqrt(dx * dx + dy * dy);
      if (dist <= maxRadius) {
        data.totalFlux += pixel;
        data.pixelsByDistance.push_back({dist, pixel});
      }
    }
  }

  return data;
}

// Forward declaration for overload
inline BackgroundStats computeBackgroundStats(const cv::Mat& starRegion,
                                               const cv::Point2f& center,
                                               int minBackgroundRadius,
                                               int maxBackgroundRadius);

// Compute background statistics from circular annulus beyond star PSF
// Samples pixels in annulus at radius [minRadius, maxRadius] from center
inline BackgroundStats computeBackgroundStats(const cv::Mat& starRegion,
                                               int minBackgroundRadius = kMinBackgroundRadius,
                                               int maxBackgroundRadius = kMaxBackgroundRadius) {
  const cv::Point2f center(starRegion.cols / 2.0f, starRegion.rows / 2.0f);

  return computeBackgroundStats(starRegion, center, minBackgroundRadius, maxBackgroundRadius);
}

inline BackgroundStats computeBackgroundStats(const cv::Mat& starRegion,
                                               const cv::Point2f& center,
                                               int minBackgroundRadius,
                                               int maxBackgroundRadius) {

  // create a ring between the radius of min/max background radii
  cv::Mat mask = cv::Mat::zeros(starRegion.size(), CV_8U);
  cv::circle(mask, center, maxBackgroundRadius, cv::Scalar(255), -1);  // Outer circle (filled)
  cv::circle(mask, center, minBackgroundRadius, cv::Scalar(0), -1);    // Inner circle (filled, erases center)

  // Extract non-zero mask pixels using OpenCV
  std::vector<cv::Point> locations;
  cv::findNonZero(mask, locations);

  if (locations.empty()) {
    return BackgroundStats{0.0, 0.0};
  }

  // Collect pixel values at mask locations
  std::vector<double> backgroundPixels;
  backgroundPixels.reserve(locations.size());
  for (const auto& pt : locations) {
    backgroundPixels.push_back(static_cast<double>(starRegion.at<uint16_t>(pt)));
  }

  // Compute median for background level
  const double backgroundLevel = computeMedian(backgroundPixels);

  // Robust sigma clipping around the median
  // This removes hot pixels / nearby faint stars from the background annulus
  const std::vector<double> clippedPixels = computeSigmaClipping(
      backgroundPixels, backgroundLevel, kClippingSigma, kClippingMaxIterations);

  if (clippedPixels.empty()) {
    // Fallback to initial statistics if all pixels were clipped
    const auto initialStats = calculateMeanAndStdDev(backgroundPixels);
    return BackgroundStats{backgroundLevel, initialStats.second};
  }

  // Compute final mean and standard deviation from clipped data
  const auto clippedStats = calculateMeanAndStdDev(clippedPixels);

  // Use mean after robust estimation
  return BackgroundStats{clippedStats.first, clippedStats.second};
}

// Compute intensity-weighted centroid using pixels above threshold
inline cv::Point2f computeCentroid(const cv::Mat& starRegion,
                                    const BackgroundStats& background,
                                    float backgroundStdDevMultiplier = kBackgroundSigmaThreshold) {
  if (background.stddev <= 0.0) {
    return cv::Point2f(starRegion.cols / 2.0f, starRegion.rows / 2.0f);
  }

  const double inclusionThreshold = background.level + backgroundStdDevMultiplier * background.stddev;

  double sumIntensity = 0.0;
  double sumX = 0.0;
  double sumY = 0.0;

  for (int y = 0; y < starRegion.rows; y++) {
    for (int x = 0; x < starRegion.cols; x++) {
      double pixelValue = starRegion.at<uint16_t>(y, x);
      double weight = computeWeight(pixelValue, inclusionThreshold, background.level);
      if (weight > 0.0) {
        sumIntensity += weight;
        sumX += weight * (x + 0.5);  // Use pixel center
        sumY += weight * (y + 0.5);
      }
    }
  }

  if (sumIntensity <= 0.0) {
    // Return center of region if no pixels above threshold
    return cv::Point2f(starRegion.cols / 2.0f, starRegion.rows / 2.0f);
  }

  return cv::Point2f(static_cast<float>(sumX / sumIntensity),
                     static_cast<float>(sumY / sumIntensity));
}

// Compute background and centroid with iterative refinement
// First pass around region center, second pass around measured centroid
inline BackgroundAndCentroid computeBackgroundAndCentroid(
    const cv::Mat& starRegion,
    float backgroundStdDevMultiplier = kBackgroundSigmaThreshold,
    int minBackgroundRadius = kMinBackgroundRadius,
    int maxBackgroundRadius = kMaxBackgroundRadius) {
  // First pass: background annulus around crop center.
  BackgroundStats background =
      computeBackgroundStats(starRegion, minBackgroundRadius, maxBackgroundRadius);

  cv::Point2f centroid =
      computeCentroid(starRegion, background, backgroundStdDevMultiplier);

  // Second pass: background annulus around measured centroid.
  background =
      computeBackgroundStats(starRegion, centroid, minBackgroundRadius, maxBackgroundRadius);

  centroid =
      computeCentroid(starRegion, background, backgroundStdDevMultiplier);

  return BackgroundAndCentroid{background, centroid};
}

// Compute HFD (Half-Flux Diameter) using pixel-by-pixel method
// Diameter of circle containing 50% of total flux
inline float computeHFD(const cv::Mat& starRegion,
                        const cv::Point2f& centroid,
                        const BackgroundStats& background,
                        double maxApertureRadius = kMaxApertureRadius,
                        float backgroundStdDevMultiplier = kHFDBackgroundMultiplier) {
  cv::Mat floatMat;
  starRegion.convertTo(floatMat, CV_32FC1);
  floatMat -= static_cast<float>(background.level);

  // Collect all pixels within maxApertureRadius aperture
  auto apertureData = collectAperturePixels(floatMat, centroid, maxApertureRadius);

  if (apertureData.totalFlux <= 0.0) {
    return 0.0f;
  }

  double halfFlux = apertureData.totalFlux / 2.0;

  // Pixel-by-pixel accumulation method
  // Sort pixels by distance from centroid and accumulate flux
  std::sort(apertureData.pixelsByDistance.begin(), apertureData.pixelsByDistance.end());

  double cumulativeFlux = 0.0;
  double previousCumulativeFlux = 0.0;
  double previousDistance = 0.0;

  for (const auto& [dist, pixel] : apertureData.pixelsByDistance) {
    cumulativeFlux += pixel;

    if (cumulativeFlux >= halfFlux) {
      // Interpolate the half-flux crossing instead of returning the distance
      // of the pixel that pushed the cumulative flux over 50%.
      double crossingFlux = cumulativeFlux - previousCumulativeFlux;
      double fraction =
          crossingFlux > 0.0 ? (halfFlux - previousCumulativeFlux) / crossingFlux : 0.0;

      double hfr = previousDistance + fraction * (dist - previousDistance);
      return static_cast<float>(hfr * 2.0f);  // Return diameter
    }

    previousCumulativeFlux = cumulativeFlux;
    previousDistance = dist;
  }

  return 0.0f;
}

// Find the brightest region in an image (returns bounding box around brightest pixel)
// Note: Very simplistic - real apps need proper star detection
inline cv::Rect findBrightestRegion(const cv::Mat& image, int windowSize = 50) {
  double minVal, maxVal;
  cv::Point minLoc, maxLoc;
  cv::minMaxLoc(image, &minVal, &maxVal, &minLoc, &maxLoc);

  // Create bounding box around the brightest pixel
  int halfWindow = windowSize / 2;
  int x = std::max(0, maxLoc.x - halfWindow);
  int y = std::max(0, maxLoc.y - halfWindow);
  int width = std::min(windowSize, image.cols - x);
  int height = std::min(windowSize, image.rows - y);

  return cv::Rect(x, y, width, height);
}
