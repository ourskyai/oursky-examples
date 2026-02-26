#include "routes.h"
#include "image_processor.h"
#include "models.h"
#include "snpe_worker.h"

#include <crow.h>

static crow::response json_response(int status, const nlohmann::json& body) {
    auto resp = crow::response(status, body.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

static crow::response error_response(int status, const std::string& message) {
    return json_response(status, ErrorResponse{message});
}

void register_routes(crow::SimpleApp& app, SnpeWorker& worker) {

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
}
