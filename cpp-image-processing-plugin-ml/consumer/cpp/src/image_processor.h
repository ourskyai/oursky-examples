#pragma once

#include <nlohmann/json.hpp>
#include <string>

class SnpeWorker;

struct ProcessResult {
    bool success;
    std::string error;
    nlohmann::json classifications; // JSON array of classification results
};

ProcessResult process_image(SnpeWorker& worker,
                            const std::string& image_path,
                            double timeout_seconds);
