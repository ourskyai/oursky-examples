//
// Created by connor on 3/6/25.
//

#include "client.hpp"
#include <iostream>

int main() {
    OSLowLevelSdkClient client("INSERT_CONTROLLER_IP_HERE:50099");
    // 0 minimumIntervalMicroseconds yields a message as soon as it's available
    client.StreamObservatoryStatus(0, 1000, [](oslowlevelsdk::V1ObservatoryStatus status) {
        std::cout << "Timestamp: " << status.timestamp().seconds()  << "." << status.timestamp().nanos() << std::endl;
        std::cout << "Primary Mount Encoder: " << status.encoder_mount_primary_radians() << " radians" << std::endl;
        std::cout << "Secondary Mount Encoder: " << status.encoder_mount_secondary_radians() << " radians" << std::endl;
        std::cout << "------------------------------------" << std::endl;
    });

    return 0;
}
