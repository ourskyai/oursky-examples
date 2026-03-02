#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <SNPE/DlContainer/IDlContainer.hpp>
#include <SNPE/DlSystem/TensorShape.hpp>
#include <SNPE/SNPE/SNPE.hpp>

struct Classification {
    int index;
    float confidence;
    std::string label;
};

class SnpeWorker {
public:
    SnpeWorker(const std::string& dlc_path,
               const std::string& labels_path,
               DlSystem::Runtime_t runtime = DlSystem::Runtime_t::CPU_FLOAT32);

    std::vector<Classification> infer(const std::vector<float>& image_data,
                                      size_t top_n = 5);

    size_t input_size() const { return input_size_; }
    size_t input_height() const { return input_height_; }
    size_t input_width() const { return input_width_; }
    size_t input_channels() const { return input_channels_; }

private:
    std::vector<std::string> load_labels(const std::string& path);
    std::vector<Classification> top_results(const float* scores, size_t count,
                                            size_t top_n);

    std::unique_ptr<SNPE::SNPE> snpe_;
    DlSystem::TensorShape input_shape_;
    std::vector<std::string> labels_;
    size_t input_size_ = 0;
    size_t input_height_ = 0;
    size_t input_width_ = 0;
    size_t input_channels_ = 0;
    std::mutex mutex_;
};
