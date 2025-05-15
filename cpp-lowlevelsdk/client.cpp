//
// Created by connor on 3/6/25.
//

#include "client.hpp"

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>

#include "oslowlevelsdk.grpc.pb.h"

using oslowlevelsdk::AxesStatus;
using oslowlevelsdk::AxesStatusRequest;

OSLowLevelSdkClient::OSLowLevelSdkClient(const std::string& server_address) {
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

    this->stub_ = oslowlevelsdk::OSLowLevelSdkService::NewStub(channel);
}

void OSLowLevelSdkClient::StreamAxesStatus(int interval_microseconds, bool includeDetailed,
                                           const std::function<void(AxesStatus)>& callback) {
    AxesStatusRequest request;
    request.set_minimum_interval_microseconds(interval_microseconds);
    request.set_include_detailed_axis_status(includeDetailed);

    grpc::ClientContext context;
    std::unique_ptr<grpc::ClientReader<AxesStatus>> reader(stub_->StreamAxesStatus(&context, request));

    AxesStatus response;
    while (reader->Read(&response)) {
        callback(response);
        // Print out the response fields
        std::cout << "Timestamp: " << response.timestamp().seconds() << std::endl;
        std::cout << "Encoder Azimuth: " << response.encoder_mount_axis0_radians() << " radians" << std::endl;
        std::cout << "Encoder Elevation: " << response.encoder_mount_axis1_radians() << " radians" << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    grpc::Status status = reader->Finish();
    if (!status.ok()) {
        std::cerr << "Stream failed: " << status.error_message() << std::endl;
    }
}