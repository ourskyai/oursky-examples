/** External Dependencies */
#include <flatbuffers/flatbuffers.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/endian/conversion.hpp>
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

static constexpr uint32_t kMaxFrameBytes = 128u << 20;

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

static bool readData(boost::asio::ip::tcp::socket& sock, void* data, std::size_t n) {
  boost::system::error_code ec;
  boost::asio::read(sock, boost::asio::buffer(data, n), ec);
  if (ec) {
    std::cerr << "TCP read error: " << ec.message() << "\n";
    return false;
  }
  return true;
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

  // Choose a camera (prefer ZWO if present)
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

  // Start continuous image capture
  std::cout << "Starting continuous exposure..." << std::endl;

  nlohmann::json captureRequestBodyDocument = {{"cameraId", chosenCameraIdentifierString},
                                               {"exposureSeconds", 1.0},
                                               {"gain", 50},
                                               {"binning", 2},
                                               {"flatCorrection", false},
                                               {"darkCorrection", false},
                                               {"plateSolve", false},
                                               {"createStretchedJPEG", false},
                                               {"createStretchedJPEGThumbnail", false}};

  std::optional<HttpResponseData> continuousExposureResponse =
      performHttpRequest(serverHostName, serverPortString, boost::beast::http::verb::post,
                         "/sensor-package/v1/start-continuous-image-capture",
                         std::optional<std::string>{captureRequestBodyDocument.dump()});

  if (!continuousExposureResponse || continuousExposureResponse->statusCode != 200) {
    std::cerr << "Continuous exposure failed." << std::endl;
    return EXIT_FAILURE;
  }

  std::string responseBodyText(continuousExposureResponse->bodyBytes.begin(),
                               continuousExposureResponse->bodyBytes.end());

  nlohmann::json responseDocument = nlohmann::json::parse(responseBodyText);

  bool successResponse = responseDocument.value("success", false);
  if (!successResponse) {
    std::cerr << "Continuous exposure failed. Response: " << responseBodyText << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Continuous exposure started successfully!" << std::endl;
  std::cout << "Response: " << responseBodyText << std::endl;

  boost::asio::io_context streamIoContext;
  boost::asio::ip::tcp::acceptor frameListenerAcceptor{
      streamIoContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 0)};
  const auto port = frameListenerAcceptor.local_endpoint().port();

  std::string streamUrl = "tcp://127.0.0.1:" + std::to_string(port);
  std::cout << "Stream URL: " << streamUrl << std::endl;

  nlohmann::json streamRequestBodyDocument = {{"streamReceiverUrl", streamUrl}};

  std::optional<HttpResponseData> streamResponse = performHttpRequest(
      serverHostName, serverPortString, boost::beast::http::verb::post, "/sensor-package/v1/start-stream-frames",
      std::optional<std::string>{streamRequestBodyDocument.dump()});

  if (!streamResponse || streamResponse->statusCode != 200) {
    std::cerr << "Streaming failed to start." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Streaming started successfully! Waiting for TCP connection..." << std::endl;

  boost::asio::ip::tcp::socket frameSocket{streamIoContext};
  frameListenerAcceptor.accept(frameSocket);  // blocks until your sender connects to tcp://127.0.0.1:<port>
  std::cout << "Sender connected from " << frameSocket.remote_endpoint() << std::endl;

  frameSocket.set_option(boost::asio::ip::tcp::no_delay(true));

  uint64_t frameIndex = 0;

  for (;;) {
    uint32_t payloadLength = 0;
    if (!readData(frameSocket, &payloadLength, sizeof(payloadLength))) {
      std::cerr << "  - payload length: FAILED (socket read).\n";
      break;
    }
    if (payloadLength == 0 || payloadLength > kMaxFrameBytes) {
      std::cerr << "  - payload length: FAILED (invalid size " << payloadLength << ").\n";
      continue;
    }

    // keep prefix + payload contiguous in `payload`
    std::vector<uint8_t> payload(sizeof(payloadLength) + payloadLength);
    payload[0] = static_cast<uint8_t>(payloadLength & 0xFF);
    payload[1] = static_cast<uint8_t>((payloadLength >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>((payloadLength >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((payloadLength >> 24) & 0xFF);

    if (!readData(frameSocket, payload.data() + sizeof(payloadLength), payloadLength)) {
      std::cerr << "  - payload read: FAILED (socket read).\n";
      break;
    }

    std::vector<uint8_t>& flatBufferByteVector = payload;

    // identifier check (pointer includes size prefix)
    if (!flatbuffers::BufferHasIdentifier(flatBufferByteVector.data(), "OSSP", /*size_prefixed=*/true)) {
      std::cerr << "  - payload identifier: FAILED (expected OSSP).\n";
      continue;
    }

    // *** Use GENERATED verify for size-prefixed buffer ***
    flatbuffers::Verifier verifier(flatBufferByteVector.data(), flatBufferByteVector.size());
    if (!hwdaemon::VerifySizePrefixedImageResultBuffer(verifier)) {
      std::cerr << "  - verification: FAILED (invalid FlatBuffer).\n";
      continue;
    }

    // *** Use GENERATED getter for size-prefixed root ***
    const hwdaemon::ImageResult* imageResultTable = hwdaemon::GetSizePrefixedImageResult(flatBufferByteVector.data());

    const hwdaemon::ImageMetadata* imageMetadataTable = imageResultTable->metadata();
    if (!imageMetadataTable) {
      std::cout << "  - metadata: not present\n";
      continue;
    }

    std::cout << "  - metadata.width: " << imageMetadataTable->width() << "\n";
    std::cout << "  - metadata.height: " << imageMetadataTable->height() << "\n";
    if (auto id = imageMetadataTable->image_id()) {
      std::cout << "  - metadata.image_id: " << id->str() << "\n";
    }

    std::cout << "Frame #" << frameIndex++ << " : " << flatBufferByteVector.size() << " bytes\n";
  }

  boost::system::error_code ec;
  frameSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  frameSocket.close(ec);

  return EXIT_SUCCESS;
}
