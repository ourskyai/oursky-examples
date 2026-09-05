#pragma once

#include <optional>

#include <opencv2/core.hpp>

#include "os_image_proc/NoInstances.hpp"

struct SourceExtractorResult {
  float ee50Diameter;
  float centroidX;
  float centroidY;
  float peak;
  float backgroundRms;
  float skyLevel;
  float totalFlux;
  int sourceCount;
};

class SourceExtractorMeasurement : NoInstances {
 public:
  static constexpr int kBackgroundTile = 64; // from unc ee50.py line 41
  static constexpr int kBackgroundFilterTiles = 3; // source extractor default
  static constexpr float kDetectionThresholdSigma = 5.0f; // from unc ee50.py line 42
  static constexpr int kMinDetectionArea = 5; // source extractor default
  static constexpr int kDeblendThresholds = 4; // from unc ee50.py line 43
  static constexpr double kDeblendContrast = 0.005; // source extractor default
  static constexpr double kPeakFloorAbsolute = 100.0; // from unc ee50.py line 44
  static constexpr double kPeakFloorRmsMultiple = 50.0; // from unc ee50.py line 44
  static constexpr double kPeakCeiling = 60000.0; // from unc ee50.py line 45
  static constexpr double kAnnulusInnerRadius = 20.0; // from unc ee50.py line 59
  static constexpr double kAnnulusOuterRadius = 25.0; // from unc ee50.py line 59
  static constexpr double kMaxApertureRadius = 20.0; // from unc ee50.py line 65
  static constexpr double kApertureRadiusStep = 0.5; // from unc ee50.py line 65

  [[nodiscard]] static std::optional<SourceExtractorResult> measure(const cv::Mat& image);
};
