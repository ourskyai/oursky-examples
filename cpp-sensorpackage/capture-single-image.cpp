/** External Dependencies */
#include <flatbuffers/flatbuffers.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

/** Generated FlatBuffers Headers */
#include "ImageResult_generated.h"

/** Standard Library Dependencies */
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct HttpResponseData {
  unsigned statusCode = 0;
  std::vector<uint8_t> bodyBytes;
};

std::optional<HttpResponseData> performHttpRequest(const std::string& serverHostName,
                                                   const std::string& serverPortString,
                                                   boost::beast::http::verb httpMethod,
                                                   const std::string& resourceTargetPath,
                                                   std::optional<std::string> requestBodyTextOptional = std::nullopt) {
  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::resolver tcpResolver{ioContext};
    boost::beast::tcp_stream tcpStream{ioContext};

    auto const resolverResults = tcpResolver.resolve(serverHostName, serverPortString);
    tcpStream.connect(resolverResults);

    boost::beast::http::request<boost::beast::http::string_body> httpRequestMessage{httpMethod, resourceTargetPath, 11};
    httpRequestMessage.set(boost::beast::http::field::host, serverHostName);
    httpRequestMessage.set(boost::beast::http::field::accept_encoding, "identity");  // avoid gzip on binaries
    if (requestBodyTextOptional) {
      httpRequestMessage.set(boost::beast::http::field::content_type, "application/json");
      httpRequestMessage.body() = *requestBodyTextOptional;
      httpRequestMessage.prepare_payload();  // sets Content-Length
    }
    httpRequestMessage.keep_alive(false);

    boost::beast::http::write(tcpStream, httpRequestMessage);

    boost::beast::flat_buffer readBuffer;
    boost::beast::http::response_parser<boost::beast::http::string_body> httpResponseParser;
    httpResponseParser.body_limit(static_cast<std::uint64_t>(-1));  // unlimited for large binaries

    boost::beast::http::read(tcpStream, readBuffer, httpResponseParser);

    boost::beast::error_code shutdownErrorCode;
    tcpStream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, shutdownErrorCode);

    auto httpResponseMessage = httpResponseParser.release();

    HttpResponseData httpResponseData;
    httpResponseData.statusCode = httpResponseMessage.result_int();

    const std::string& responseBodyString = httpResponseMessage.body();
    httpResponseData.bodyBytes.assign(
        reinterpret_cast<const uint8_t*>(responseBodyString.data()),
        reinterpret_cast<const uint8_t*>(responseBodyString.data()) + responseBodyString.size());

    return httpResponseData;
  } catch (...) {
    return std::nullopt;
  }
}

int main() {
  const std::string serverHostName = "localhost";
  const std::string serverPortString = "9080";

  // Query connected cameras
  std::cout << "Querying connected cameras..." << std::endl;

  std::optional<HttpResponseData> connectedCamerasResponse = performHttpRequest(
      serverHostName, serverPortString, boost::beast::http::verb::get, "/sensor-package/v1/connected-cameras");

  if (!connectedCamerasResponse || connectedCamerasResponse->statusCode != 200) {
    std::cerr << "Failed to fetch connected cameras." << std::endl;
    return EXIT_FAILURE;
  }

  std::string connectedCamerasResponseBodyText(connectedCamerasResponse->bodyBytes.begin(),
                                               connectedCamerasResponse->bodyBytes.end());

  nlohmann::json connectedCamerasDocument = nlohmann::json::parse(connectedCamerasResponseBodyText);

  if (!connectedCamerasDocument.is_array() || connectedCamerasDocument.empty()) {
    std::cerr << "No cameras found. Exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Connected cameras (" << connectedCamerasDocument.size() << "):" << std::endl;
  for (const nlohmann::json& cameraObject : connectedCamerasDocument) {
    if (cameraObject.is_object()) {
      const std::string cameraIdentifierString = cameraObject.value("id", std::string{});
      const std::string cameraDisplayNameString = cameraObject.value("name", std::string{});
      std::cout << "  - id: \"" << cameraIdentifierString << "\""
                << (cameraDisplayNameString.empty() ? "" : std::string{", name: \"" + cameraDisplayNameString + "\""})
                << std::endl;
    }
  }

  // Choose a camera (prefer QHY if present)
  std::string chosenCameraIdentifierString = connectedCamerasDocument.front().value("id", std::string{});
  for (const nlohmann::json& cameraObject : connectedCamerasDocument) {
    if (cameraObject.is_object()) {
      const std::string cameraIdentifierString = cameraObject.value("id", std::string{});
      if (cameraIdentifierString.find("QHY") != std::string::npos) {
        chosenCameraIdentifierString = cameraIdentifierString;
        break;
      }
    }
  }

  if (chosenCameraIdentifierString.empty()) {
    std::cerr << "No usable camera identifier. Exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Using camera: " << chosenCameraIdentifierString << std::endl;

  // Capture a single image → FlatBuffer bytes
  std::cout << "Capturing single image..." << std::endl;

  nlohmann::json captureRequestBodyDocument = {{"cameraId", chosenCameraIdentifierString},
                                               {"exposureSeconds", 1.0},
                                               {"gain", 50},
                                               {"binning", 1},
                                               {"offset", 1},
                                               {"gainMode", 1}};

  std::optional<HttpResponseData> imageCaptureResponse = performHttpRequest(
      serverHostName, serverPortString, boost::beast::http::verb::post, "/sensor-package/v1/capture-image",
      std::optional<std::string>{captureRequestBodyDocument.dump()});

  if (!imageCaptureResponse || imageCaptureResponse->statusCode != 200) {
    std::cerr << "Image capture failed." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Capture results:" << std::endl;
  std::cout << "  - payloadBytes: " << imageCaptureResponse->bodyBytes.size() << std::endl;

  // Verify FlatBuffer and deserialize it
  std::vector<uint8_t>& flatBufferByteVector = imageCaptureResponse->bodyBytes;

  // identifier check (pointer includes size prefix)
  if (!flatbuffers::BufferHasIdentifier(flatBufferByteVector.data(), "OSSP", /*size_prefixed=*/true)) {
    std::cerr << "  - payload identifier: FAILED (expected OSSP). Exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  // Use GENERATED verify for size-prefixed buffer
  flatbuffers::Verifier flatBufferVerifier(flatBufferByteVector.data(), flatBufferByteVector.size());
  bool flatBufferIsValid = hwdaemon::VerifySizePrefixedImageResultBuffer(flatBufferVerifier);
  if (!flatBufferIsValid) {
    std::cerr << "  - verification: FAILED (invalid FlatBuffer). Exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  // Use GENERATED getter for size-prefixed root
  const hwdaemon::ImageResult* imageResultTable = hwdaemon::GetSizePrefixedImageResult(flatBufferByteVector.data());
  const hwdaemon::ImageMetadata* imageMetadataTable = imageResultTable->metadata();

  if (!imageMetadataTable) {
    std::cout << "  - metadata: not present" << std::endl;
    return EXIT_SUCCESS;
  }

  std::cout << "  - metadata.width: " << imageMetadataTable->width() << std::endl;
  std::cout << "  - metadata.height: " << imageMetadataTable->height() << std::endl;
  std::cout << "  - metadata.image_id: " << imageMetadataTable->image_id()->str() << std::endl;

  return EXIT_SUCCESS;
}
