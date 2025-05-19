#include "client.hpp"

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <chrono>
#include <fstream>

#include "oslowlevelsdk.grpc.pb.h"
#include "grpcpp/channel.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"

std::string readPemFile(const std::string& file_path) {
    std::ifstream file(file_path);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

OSLowLevelSdkClient::OSLowLevelSdkClient(const std::string &serverAddress) {
    grpc::SslCredentialsOptions sslOptions;

    // Optional: if you cannot add the observable space root cert to the system
    // Load the server's cert (as a trusted root)
    sslOptions.pem_root_certs = readPemFile("/home/connor/git/github.com/oursky/oursky-examples/cpp-lowlevelsdk/oursky_root.crt");

    // Load the client's cert chain and private key (for mutual TLS)
    sslOptions.pem_cert_chain = readPemFile("/home/connor/git/github.com/oursky/oursky-examples/cpp-lowlevelsdk/client_cert.pem");
    sslOptions.pem_private_key = readPemFile("/home/connor/git/github.com/oursky/oursky-examples/cpp-lowlevelsdk/client_key.pem");

    // Set SSL Credentials for mutual TLS
    auto credentials = grpc::SslCredentials(sslOptions);

    // Override target name if cert CN or SAN differs from actual address
    grpc::ChannelArguments args;
    args.SetSslTargetNameOverride("9190cf50-1373-4910-b549-cb9caa99b283.nodes.prod.oursky.ai");

    auto channel = grpc::CreateCustomChannel(serverAddress, credentials, args);
    this->stub = oslowlevelsdk::ObservatoryService::NewStub(channel);
}

void OSLowLevelSdkClient::StreamObservatoryStatus(int minimumIntervalMicroseconds, int timeoutMilliseconds,
                                                  const std::function<void(
                                                      oslowlevelsdk::V1ObservatoryStatus)> &callback) {
  oslowlevelsdk::V1StreamObservatoryStatusRequest request;
  request.set_minimum_interval_microseconds(minimumIntervalMicroseconds);

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::minutes(120));

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
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);  // <- per-read timeout
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
