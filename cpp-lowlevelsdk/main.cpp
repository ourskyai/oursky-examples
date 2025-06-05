#include "client.hpp"
#include <iostream>
#include <cstdio>
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: grpc_client [ipaddress:port]" << std::endl;
        return 1;
    }

    std::cerr << "Connecting" << std::endl;
    OSLowLevelSdkClient client(argv[1]);
    std::cerr << "Connected" << std::endl;

    std::cout << "# NumReceivedMessages, DC Time (usec since 1970), Received time (usec since 1970), Timestamp Age (usec), Timestamp Interval (msec)\n";
    double lastMessageTimeSeconds = 0;
    int minInterval = 0;
    int timeout = 10000;
    int messageCount = 0;
    client.StreamObservatoryStatus(minInterval, timeout, [&lastMessageTimeSeconds, &messageCount](oslowlevelsdk::V1ObservatoryStatus status) {
        auto receivedSystemTimeNow = std::chrono::system_clock::now();
        auto receivedSystemTimeUsec = std::chrono::duration_cast<std::chrono::microseconds>(receivedSystemTimeNow.time_since_epoch()).count();

        double messageDcTimeSeconds = status.timestamp().seconds() + (status.timestamp().nanos() / 1000000000.0);
        long messageDcTimeUsec = status.timestamp().seconds() * 1000000 + (status.timestamp().nanos() / 1000);

        long timestampAgeUsec = receivedSystemTimeUsec - messageDcTimeUsec;

        if (lastMessageTimeSeconds > 0) {
            double messageTimestampIntervalMilliseconds = (messageDcTimeSeconds - lastMessageTimeSeconds) * 1000;
            printf("%d, %ld, %ld, %ld, %f\n", 
                    messageCount, 
                    messageDcTimeUsec, 
                    receivedSystemTimeUsec,
                    timestampAgeUsec,
                    messageTimestampIntervalMilliseconds
                    );
        }
        messageCount++;
        lastMessageTimeSeconds = messageDcTimeSeconds;
    });

    return 0;
}
