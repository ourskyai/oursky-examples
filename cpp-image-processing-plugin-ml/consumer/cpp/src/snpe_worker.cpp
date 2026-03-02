#include "snpe_worker.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>

#include <SNPE/SNPE/SNPEBuilder.hpp>
#include <SNPE/SNPE/SNPEFactory.hpp>

SnpeWorker::SnpeWorker(const std::string& dlc_path,
                       const std::string& labels_path,
                       DlSystem::Runtime_t runtime) {
    auto container = DlContainer::IDlContainer::open(dlc_path);
    if (!container) {
        throw std::runtime_error("Failed to open DLC: " + dlc_path);
    }

    // Verify the requested runtime is available on this platform
    if (!SNPE::SNPEFactory::isRuntimeAvailable(runtime)) {
        std::string name = DlSystem::RuntimeList::runtimeToString(runtime);
        throw std::runtime_error("Runtime not available: " + name);
    }

    SNPE::SNPEBuilder builder(container.get());
    DlSystem::RuntimeList runtime_list(runtime);
    builder.setRuntimeProcessorOrder(runtime_list)
           .setPerformanceProfile(DlSystem::PerformanceProfile_t::HIGH_PERFORMANCE);

    snpe_ = builder.build();
    if (!snpe_) {
        throw std::runtime_error(
            std::string("Failed to build SNPE: ") + SNPE::SNPEFactory::getLastError());
    }

    // Cache input shape for reuse during inference
    auto dims = snpe_->getInputDimensions();
    if (!dims) {
        throw std::runtime_error("Failed to query input dimensions");
    }
    input_shape_ = *dims;

    input_size_ = 1;
    for (size_t i = 0; i < input_shape_.rank(); ++i) {
        input_size_ *= input_shape_[i];
    }

    // Extract spatial dimensions assuming NHWC (4D) or HWC (3D) layout
    if (input_shape_.rank() == 4) {
        input_height_   = input_shape_[1];
        input_width_    = input_shape_[2];
        input_channels_ = input_shape_[3];
    } else if (input_shape_.rank() == 3) {
        input_height_   = input_shape_[0];
        input_width_    = input_shape_[1];
        input_channels_ = input_shape_[2];
    }

    std::cout << "SNPE ready — input " << input_height_ << "x"
              << input_width_ << "x" << input_channels_
              << " (" << input_size_ << " floats)" << std::endl;

    labels_ = load_labels(labels_path);
}

std::vector<Classification> SnpeWorker::infer(const std::vector<float>& image_data,
                                              size_t top_n) {
    if (image_data.size() != input_size_) {
        throw std::runtime_error(
            "Input size mismatch: expected " + std::to_string(input_size_) +
            ", got " + std::to_string(image_data.size()));
    }

    // Create empty tensor and copy float data in via iterators
    // (the raw-data overload of createTensor is for special formats like NV21)
    auto& factory = SNPE::SNPEFactory::getTensorFactory();
    auto input_tensor = factory.createTensor(input_shape_);
    if (!input_tensor) {
        throw std::runtime_error("Failed to create input tensor");
    }
    std::copy(image_data.begin(), image_data.end(), input_tensor->begin());

    DlSystem::TensorMap output_map;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!snpe_->execute(input_tensor.get(), output_map)) {
            throw std::runtime_error(
                std::string("SNPE execute failed: ") + SNPE::SNPEFactory::getLastError());
        }
    }

    auto names = output_map.getTensorNames();
    if (names.size() == 0) {
        throw std::runtime_error("SNPE returned no output tensors");
    }

    auto* output_tensor = output_map.getTensor(names.at(0));
    if (!output_tensor) {
        throw std::runtime_error("Failed to retrieve output tensor");
    }

    return top_results(&(*output_tensor->begin()),
                       output_tensor->getSize(), top_n);
}

std::vector<std::string> SnpeWorker::load_labels(const std::string& path) {
    std::vector<std::string> labels;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open labels file: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        labels.push_back(line);
    }
    return labels;
}

std::vector<Classification> SnpeWorker::top_results(const float* scores,
                                                    size_t count,
                                                    size_t top_n) {
    std::vector<int> indices(count);
    std::iota(indices.begin(), indices.end(), 0);
    size_t n = std::min(top_n, count);
    std::partial_sort(indices.begin(), indices.begin() + n, indices.end(),
                      [scores](int a, int b) { return scores[a] > scores[b]; });

    std::vector<Classification> results;
    results.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        int idx = indices[i];
        std::string label = (static_cast<size_t>(idx) < labels_.size())
                                ? labels_[idx]
                                : "unknown";
        results.push_back({idx, scores[idx], label});
    }
    return results;
}
