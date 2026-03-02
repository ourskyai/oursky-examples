#include "image_processor.h"
#include "models.h"
#include "snpe_worker.h"

#include <SNPE/DlSystem/DlEnums.hpp>

#include <crow.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

static std::string env_or(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

static DlSystem::Runtime_t parse_runtime(const std::string& name) {
    static const std::unordered_map<std::string, DlSystem::Runtime_t> map = {
        {"cpu",    DlSystem::Runtime_t::CPU_FLOAT32},
        {"gpu",    DlSystem::Runtime_t::GPU_FLOAT32_16_HYBRID},
        {"gpu16",  DlSystem::Runtime_t::GPU_FLOAT16},
        {"dsp",    DlSystem::Runtime_t::DSP_FIXED8_TF},
        {"aip",    DlSystem::Runtime_t::AIP_FIXED8_TF},
    };

    auto it = map.find(name);
    if (it != map.end()) return it->second;

    std::cerr << "Unknown SNPE_RUNTIME '" << name
              << "'. Options: cpu, gpu, gpu16, dsp, aip" << std::endl;
    std::exit(1);
}

static crow::response json_response(int status, const nlohmann::json& body) {
    auto resp = crow::response(status, body.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

static crow::response error_response(int status, const std::string& message) {
    return json_response(status, ErrorResponse{message});
}

int main() {
    std::string model_path  = env_or("MODEL_PATH",  "prerequisites/models/inception_v3.dlc");
    std::string labels_path = env_or("LABELS_PATH", "prerequisites/imagenet_slim_labels.txt");
    std::string runtime_str = env_or("SNPE_RUNTIME", "cpu");
    int port = std::atoi(env_or("PORT", "8099").c_str());

    auto runtime = parse_runtime(runtime_str);
    std::cout << "Runtime:  " << runtime_str << std::endl;
    std::cout << "Model:    " << model_path << std::endl;
    std::cout << "Labels:   " << labels_path << std::endl;

    SnpeWorker worker(model_path, labels_path, runtime);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/custom-image-processing/v1/images")
        .methods(crow::HTTPMethod::POST)([&worker](const crow::request& req) {
            ProcessImageRequest request;
            try {
                request = nlohmann::json::parse(req.body).get<ProcessImageRequest>();
            } catch (const nlohmann::json::exception& e) {
                return error_response(400, e.what());
            }

            if (request.timeout_seconds <= 0) {
                return error_response(400, "timeoutSeconds must be greater than 0");
            }

            auto result = process_image(
                worker,
                request.raw_image_path,
                request.timeout_seconds);

            if (!result.success) {
                return error_response(422, result.error);
            }

            ProcessImageResponse response;
            response.results = result.classifications;
            return json_response(200, response);
        });

    CROW_ROUTE(app, "/health")
        .methods(crow::HTTPMethod::GET)([]() {
            return json_response(200, HealthCheckResponse{"OK"});
        });

    std::cout << "Listening on port " << port << std::endl;
    app.port(port)
        .multithreaded()
        .run();
}
