//
// Created by connor on 3/6/25.
//

#ifndef CPP_LOWLEVELSDK_CLIENT_HPP
#define CPP_LOWLEVELSDK_CLIENT_HPP

#include <memory>

#include "oslowlevelsdk.pb.h"
#include "oslowlevelsdk.grpc.pb.h"

class OSLowLevelSdkClient {
private:
    std::unique_ptr<oslowlevelsdk::OSLowLevelSdkService::Stub> stub_;

public:
    explicit OSLowLevelSdkClient(const std::string& server_address);

    // Method to stream Axes Status updates
    void StreamAxesStatus(int interval_microseconds, bool includeDetailed,
                          const std::function<void(oslowlevelsdk::AxesStatus)>& callback);
};

#endif //CPP_LOWLEVELSDK_CLIENT_HPP
