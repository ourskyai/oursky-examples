//
// Created by connor on 3/6/25.
//

#include "client.hpp"
#include <iostream>

int main() {
    OSLowLevelSdkClient client("localhost:50099");
    client.StreamObservatoryStatus(1000, 1000, [](oslowlevelsdk::V1ObservatoryStatus status) {
        std::cout << "Timestamp: " << status.timestamp().seconds() << std::endl;
        std::cout << "Primary Mount Encoder: " << status.encoder_mount_primary_radians() << " radians" << std::endl;
        std::cout << "Secondary Mount Encoder: " << status.encoder_mount_secondary_radians() << " radians" << std::endl;
        std::cout << "------------------------------------" << std::endl;
    });

    return 0;
}