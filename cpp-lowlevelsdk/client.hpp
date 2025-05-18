#ifndef CPP_LOWLEVELSDK_CLIENT_HPP
#define CPP_LOWLEVELSDK_CLIENT_HPP

#include <memory>

#include "oslowlevelsdk.pb.h"
#include "oslowlevelsdk.grpc.pb.h"

class OSLowLevelSdkClient {
private:
    std::unique_ptr<oslowlevelsdk::ObservatoryService::Stub> stub;

public:
    explicit OSLowLevelSdkClient(const std::string& server_address);

    void StreamObservatoryStatus(int minimumIntervalMicroseconds, int timeoutMilliseconds, const std::function<void(oslowlevelsdk::V1ObservatoryStatus)>& callback);
};

#endif //CPP_LOWLEVELSDK_CLIENT_HPP
