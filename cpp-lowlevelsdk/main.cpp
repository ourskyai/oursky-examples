//
// Created by connor on 3/6/25.
//

#include "client.hpp"
#include <iostream>

int main() {
    OSLowLevelSdkClient client("localhost:50051");
    client.StreamAxesStatus(1000, true, [](oslowlevelsdk::AxesStatus status) {
        std::cout << "Timestamp: " << status.timestamp().seconds() << std::endl;
        std::cout << "Encoder Azimuth: " << status.encoder_mount_axis0_radians() << " radians" << std::endl;
        std::cout << "Encoder Elevation: " << status.encoder_mount_axis1_radians() << " radians" << std::endl;
        std::cout << "------------------------------------" << std::endl;
    });

    return 0;
}