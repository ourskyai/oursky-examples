// Demonstrates HFD computation using a simulated gaussian star image
// This is a minimal example - real usage requires FITS I/O and calibration.

#include <iostream>
#include <iomanip>
#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "demo_argus_hfd_utils.hpp"

cv::Mat makeGaussianStar(double sigma) {
  const int size = 100;
  const double cx = 50.0, cy = 50.0;
  const double peak = 50000.0;
  const double bg = 1000.0;

  std::cout << "Input:  sigma = " << sigma << " px\n\n";
  std::cout << "Generating simulated star:\n";
  std::cout << "  Image size:  " << size << "x" << size << " px\n";
  std::cout << "  Center:      (" << cx << ", " << cy << ")\n";
  std::cout << "  Peak:        " << peak << " ADU\n";
  std::cout << "  Background:  " << bg << " ADU\n";
  std::cout << "  Sigma:       " << sigma << " px\n";

  cv::Mat img(size, size, CV_16UC1);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      double dx = (x + 0.5) - cx;
      double dy = (y + 0.5) - cy;
      double r2 = dx * dx + dy * dy;
      double star = peak * std::exp(-r2 / (2.0 * sigma * sigma));
      double val = std::max(0.0, std::min(65535.0, bg + star));
      img.at<uint16_t>(y, x) = static_cast<uint16_t>(val);
    }
  }

  // Save as JPG for visualization
  cv::Mat img8bit;
  img.convertTo(img8bit, CV_8U, 255.0 / 65535.0);
  cv::imwrite("/tmp/generated_star.jpg", img8bit);
  std::cout << "  Saved:       /tmp/generated_star.jpg\n\n";

  return img;
}

void showResults(double sigma, float hfd, const BackgroundAndCentroid& result) {
  const double theoretical = 2.355 * sigma;
  const double empirical = 2.55 * sigma;

  std::cout << "Results:\n";
  std::cout << "  Background:  " << std::fixed << std::setprecision(1)
            << result.background.level << " ± " << std::setprecision(2)
            << result.background.stddev << " ADU\n";
  std::cout << "  Centroid:    (" << std::setprecision(2)
            << result.centroid.x << ", " << result.centroid.y << ")\n";
  std::cout << "  HFD:         " << std::setprecision(3) << hfd << " px\n\n";
  std::cout << "Expected:\n";
  std::cout << "  Theoretical: " << theoretical << " px  (2.355σ)\n";
  std::cout << "  Empirical:   " << empirical << " px  (2.550σ)\n";
  std::cout << "  Error:       " << std::showpos << (hfd - empirical) << std::noshowpos
            << " px (" << std::setprecision(1)
            << (100.0 * std::abs(hfd - empirical) / empirical) << "%)\n";
}

int main(int argc, char* argv[]) {
  const double sigma = 2.0;

  std::cout << "HFD Computation Demo\n\n";

  cv::Mat img = makeGaussianStar(sigma);
  cv::Rect region = findBrightestRegion(img, 50);
  cv::Mat crop = img(region);

  auto result = computeBackgroundAndCentroid(crop);
  float hfd = computeHFD(crop, result.centroid, result.background);

  showResults(sigma, hfd, result);

  return 0;
}
