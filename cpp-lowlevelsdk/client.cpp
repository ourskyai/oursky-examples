#include "client.hpp"

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <chrono>

#include "oslowlevelsdk.grpc.pb.h"
#include "grpcpp/channel.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"

OSLowLevelSdkClient::OSLowLevelSdkClient(const std::string &serverAddress) {
  grpc::SslCredentialsOptions sslOptions;
  sslOptions.pem_root_certs = "";
  sslOptions.pem_cert_chain = "";
  sslOptions.pem_private_key = "";
  auto credentials = grpc::SslCredentials(sslOptions);
  grpc::ChannelArguments args;
  args.SetSslTargetNameOverride("xyz.nodes.prod.oursky.ai");
  auto channel = grpc::CreateCustomChannel(serverAddress, credentials, args);

  this->stub = oslowlevelsdk::ObservatoryService::NewStub(channel);
}

void OSLowLevelSdkClient::StreamObservatoryStatus(int minimumIntervalMicroseconds, int timeoutMilliseconds,
                                                  const std::function<void(
                                                      oslowlevelsdk::V1ObservatoryStatus)> &callback) {
  oslowlevelsdk::V1StreamObservatoryStatusRequest request;
  request.set_minimum_interval_microseconds(minimumIntervalMicroseconds);

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(30));

  grpc::CompletionQueue cq;

  std::unique_ptr<grpc::ClientAsyncReader < oslowlevelsdk::V1ObservatoryStatus>>
  reader(
      stub->AsyncV1StreamObservatoryStatus(&context, request, &cq, reinterpret_cast<void *>(1)));

  // Wait for the stream to be initialized
  void *gotTag;
  bool ok = false;
  cq.Next(&gotTag, &ok);
  if (!ok || gotTag != reinterpret_cast<void *>(1)) {
    std::cerr << "Failed to initialize stream" << std::endl;
    return;
  }

  oslowlevelsdk::V1ObservatoryStatus response;
  while (true) {
    reader->Read(&response, reinterpret_cast<void *>(2));
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);  // <- per-read timeout
    grpc::CompletionQueue::NextStatus status = cq.AsyncNext(&gotTag, &ok, deadline);

    if (status == grpc::CompletionQueue::TIMEOUT) {
      std::cout << "Read timed out" << std::endl;
      context.TryCancel();
      break;
    }

    if (status == grpc::CompletionQueue::SHUTDOWN || !ok) {
      break;
    }

    if (gotTag == reinterpret_cast<void *>(2)) {
      std::cout << "Timestamp: " << response.timestamp().seconds() << std::endl;
      callback(response);
    }
  }

  // Finish the stream
  grpc::Status finishStatus;
  reader->Finish(&finishStatus, reinterpret_cast<void *>(3));
  cq.Next(&gotTag, &ok);

  if (!finishStatus.ok()) {
    std::cerr << "Stream ended with error: " << finishStatus.error_message() << std::endl;
  }

  cq.Shutdown();
}