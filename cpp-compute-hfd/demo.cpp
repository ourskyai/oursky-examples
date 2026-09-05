// Demonstrates Source Extractor EE50 computation using a simulated Gaussian star image.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "os_image_proc/SourceExtractorMeasurement.hpp"

namespace {

std::string trim(const std::string& value) {
  const auto first = value.find_first_not_of(' ');
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(' ') - first + 1);
}

cv::Mat loadFitsImage(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open FITS image: " + path);
  }

  int bitpix = 0;
  int width = 0;
  int height = 0;
  double bzero = 0.0;
  double bscale = 1.0;
  size_t cardCount = 0;
  bool foundEnd = false;

  std::string card(80, '\0');
  while (input.read(card.data(), static_cast<std::streamsize>(card.size()))) {
    ++cardCount;
    const std::string keyword = trim(card.substr(0, 8));
    if (keyword == "END") {
      foundEnd = true;
      break;
    }
    if (card[8] != '=') {
      continue;
    }

    std::string value = card.substr(10);
    if (const auto comment = value.find('/'); comment != std::string::npos) {
      value.erase(comment);
    }
    value = trim(value);

    if (keyword == "BITPIX") {
      bitpix = std::stoi(value);
    } else if (keyword == "NAXIS1") {
      width = std::stoi(value);
    } else if (keyword == "NAXIS2") {
      height = std::stoi(value);
    } else if (keyword == "BZERO") {
      bzero = std::stod(value);
    } else if (keyword == "BSCALE") {
      bscale = std::stod(value);
    }
  }

  if (!foundEnd || bitpix != 16 || width <= 0 || height <= 0) {
    throw std::runtime_error("Only uncompressed 16-bit primary FITS images are supported: " + path);
  }

  constexpr size_t kFitsBlockSize = 2880;
  const size_t headerBytes = cardCount * card.size();
  const size_t dataOffset = ((headerBytes + kFitsBlockSize - 1) / kFitsBlockSize) * kFitsBlockSize;
  input.seekg(static_cast<std::streamoff>(dataOffset), std::ios::beg);

  cv::Mat image(height, width, CV_16UC1);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      unsigned char bytes[2]{};
      if (!input.read(reinterpret_cast<char*>(bytes), sizeof(bytes))) {
        throw std::runtime_error("FITS image data is truncated: " + path);
      }
      const uint16_t encoded = (static_cast<uint16_t>(bytes[0]) << 8U) | bytes[1];
      const int32_t signedValue = encoded >= 0x8000U ? static_cast<int32_t>(encoded) - 65536 : encoded;
      const double scaled = std::clamp(signedValue * bscale + bzero, 0.0, 65535.0);
      image.at<uint16_t>(y, x) = static_cast<uint16_t>(std::lround(scaled));
    }
  }

  // Match OSImageProcessor::loadFitsFile; SourceExtractorMeasurement restores FITS orientation.
  cv::flip(image, image, 0);
  return image;
}

bool isFitsPath(std::string path) {
  std::transform(path.begin(), path.end(), path.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  const auto endsWith = [&path](const std::string& suffix) {
    return path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  return endsWith(".fits") || endsWith(".fit") || endsWith(".fts");
}

cv::Mat loadImage(const std::string& path) {
  if (isFitsPath(path)) {
    return loadFitsImage(path);
  }
  return cv::imread(path, cv::IMREAD_UNCHANGED);
}

}  // namespace

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

void showResults(const SourceExtractorResult& result) {
  std::cout << "Results:\n";
  std::cout << "  Sources:     " << result.sourceCount << "\n";
  std::cout << "  Sky level:   " << std::fixed << std::setprecision(2)
            << result.skyLevel << " ADU\n";
  std::cout << "  Background:  " << result.backgroundRms << " ADU RMS\n";
  std::cout << "  Centroid:    (" << std::setprecision(2)
            << result.centroidX << ", " << result.centroidY << ")\n";
  std::cout << "  Peak:        " << result.peak << " ADU\n";
  std::cout << "  Total flux:  " << result.totalFlux << " ADU\n";
  std::cout << "  EE50:        " << std::setprecision(3) << result.ee50Diameter << " px\n";
}

void showExpectedGaussian(double sigma, const SourceExtractorResult& result) {
  const double theoretical = 2.355 * sigma;

  std::cout << "\n";
  std::cout << "Expected:\n";
  std::cout << "  Gaussian:    " << theoretical << " px  (2.355σ)\n";
  std::cout << "  Error:       " << std::showpos << (result.ee50Diameter - theoretical) << std::noshowpos
            << " px (" << std::setprecision(1)
            << (100.0 * std::abs(result.ee50Diameter - theoretical) / theoretical) << "%)\n";
}

int main(int argc, char* argv[]) {
  const double sigma = 2.0;

  std::cout << "Source Extractor EE50 Computation Demo\n\n";

  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [image]\n";
    return 1;
  }

  cv::Mat img;
  const bool usingInputFile = argc == 2;
  if (usingInputFile) {
    const std::string imagePath = argv[1];
    try {
      img = loadImage(imagePath);
    } catch (const std::exception& error) {
      std::cerr << error.what() << "\n";
      return 1;
    }
    if (img.empty()) {
      std::cerr << "Failed to load image: " << imagePath << "\n";
      return 1;
    }
    if (img.channels() != 1) {
      std::cerr << "Input image must have one channel: " << imagePath << "\n";
      return 1;
    }
    std::cout << "Input:  " << imagePath << "\n";
    std::cout << "Image size:  " << img.cols << "x" << img.rows << " px\n\n";
  } else {
    img = makeGaussianStar(sigma);
  }

  const auto result = SourceExtractorMeasurement::measure(img);
  if (!result.has_value()) {
    std::cerr << "Source extractor found no source in the image\n";
    return 1;
  }

  showResults(result.value());
  if (!usingInputFile) {
    showExpectedGaussian(sigma, result.value());
  }

  return 0;
}
