#pragma once

#include <nlohmann/json.hpp>
#include <string>

// POST /custom-image-processing/v1/images - request body
struct ProcessImageRequest {
    std::string raw_image_path;
    double timeout_seconds;
};

inline void from_json(const nlohmann::json& j, ProcessImageRequest& r) {
    j.at("rawImagePath").get_to(r.raw_image_path);
    j.at("timeoutSeconds").get_to(r.timeout_seconds);
}

// POST /custom-image-processing/v1/images - 200 response
struct ProcessImageResponse {
    nlohmann::json results; // JSON array of classification results
};

inline void to_json(nlohmann::json& j, const ProcessImageResponse& r) {
    j["results"] = r.results;
}

// GET /health - 200 response
struct HealthCheckResponse {
    std::string status;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HealthCheckResponse, status)

// Error response (400, 422, 500)
struct ErrorResponse {
    std::string error;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ErrorResponse, error)
