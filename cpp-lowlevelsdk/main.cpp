#include "client.hpp"
#include <iostream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <filesystem>
#include <cmath>
#include <CLI/CLI.hpp>

void runOffsetDemo(OSLowLevelSdkClient& client, double offsetAmplitudeArcsec, int offsetIntervalMicrosec)
{
    double amplitudeRadians = offsetAmplitudeArcsec / 3600.0 * M_PI / 180;

    while (true) {
        std::cout << "Offsetting by " << amplitudeRadians << ", 0" << std::endl;
        client.ApplyMountOffsets(false, amplitudeRadians, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(offsetIntervalMicrosec));

        std::cout << "Offsetting by " << amplitudeRadians << ", " << amplitudeRadians << std::endl;
        client.ApplyMountOffsets(false, amplitudeRadians, amplitudeRadians);
        std::this_thread::sleep_for(std::chrono::milliseconds(offsetIntervalMicrosec));

        std::cout << "Offsetting by " << "0, " << amplitudeRadians << std::endl;
        client.ApplyMountOffsets(false, 0, amplitudeRadians);
        std::this_thread::sleep_for(std::chrono::milliseconds(offsetIntervalMicrosec));

        std::cout << "Offsetting by " << "0, 0" << std::endl;
        client.ApplyMountOffsets(false, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(offsetIntervalMicrosec));
    }
}

void runStreamingDemo(OSLowLevelSdkClient& client, int minIntervalMicrosec, int timeoutMillisec)
{
    std::cout << "# NumReceivedMessages, DC Time (seconds since 1970), Received time (seconds since 1970), DC Timestamp Age (msec), DC Timestamp Interval (msec), Receive time interval (msec), Azimuth, Altitude\n";

    double lastDcTimestampSeconds = 0;
    double lastReceiveTimeSeconds = 0;
    int messageCount = 0;
    client.StreamObservatoryStatus(minIntervalMicrosec, timeoutMillisec,
            [&lastDcTimestampSeconds, &lastReceiveTimeSeconds, &messageCount](oslowlevelsdk::V1ObservatoryStatus status) {
        auto receivedAtSystemTime = std::chrono::system_clock::now();
        auto receivedAtSystemTimeSeconds = std::chrono::duration<double>(receivedAtSystemTime.time_since_epoch()).count();

        double messageDcTimestampSeconds = status.timestamp().seconds() + (status.timestamp().nanos() / 1000000000.0);

        double timestampAgeMsec = (receivedAtSystemTimeSeconds - messageDcTimestampSeconds) * 1000;

        if (lastDcTimestampSeconds > 0) {
            double messageTimestampIntervalMsec = (messageDcTimestampSeconds - lastDcTimestampSeconds) * 1000;
            double receiveTimeIntervalMsec = (receivedAtSystemTimeSeconds - lastReceiveTimeSeconds) * 1000;

            double azDegs = status.apparent_mount_azimuth_radians() * 180 / 3.14159;
            double altDegs = status.apparent_mount_elevation_radians() * 180 / 3.14159;
            
            printf("%d, %.6f, %.6f, %.3f, %.3f, %.3f, %.4f, %.4f\n",
                    messageCount,
                    messageDcTimestampSeconds,
                    receivedAtSystemTimeSeconds,
                    timestampAgeMsec,
                    messageTimestampIntervalMsec,
                    receiveTimeIntervalMsec,
                    azDegs,
                    altDegs
                    );
        }

        messageCount++;
        lastDcTimestampSeconds = messageDcTimestampSeconds;
        lastReceiveTimeSeconds = receivedAtSystemTimeSeconds;
    });
}

int main(int argc, char *argv[]) {
    CLI::App app{"gRPC Client for Observatory API"};
    app.formatter(std::make_shared<CLI::Formatter>());

    std::string serverAddress;
    app.add_option("server", serverAddress, "API server address")->required();

    int port = 50099;
    app.add_option("--port", port, "API server port number (default: 50099)");

    int minIntervalMicrosec = 0;
    app.add_option("--min-interval", minIntervalMicrosec,
        "Minimum interval between streamed messages, in microseconds\n"
        "(default: 0 = as fast as possible)");

    int timeoutMillisec = 10000;
    app.add_option("--timeout", timeoutMillisec, "Timeout in milliseconds (default: 10000)");


    std::string certRootPath = "./certs";
    app.add_option("--cert-dir", certRootPath, "Path to the directory containing certificates and key")->default_str("./certs");

    std::string controllerId;
    app.add_option("--controller-id", controllerId,
        "The UUID of the controller.\n"
        "TIP: You can read this from your certificate file as follows:\n"
        "openssl x509 -in client_cert.pem -noout -text | grep nodeControllerId\n"
    )->required();

    bool demoOffsets = false;
    app.add_flag("--demo-offsets", demoOffsets, "Run offset demo");

    double offsetAmplitudeArcsec = 10;
    app.add_option("--offset-amplitude", offsetAmplitudeArcsec, "Ampltiude of offsets (arcsec)");

    int offsetIntervalMicrosec = 500;
    app.add_option("--offset-interval", offsetIntervalMicrosec, "Interval between offsets (microseconds)");

    CLI11_PARSE(app, argc, argv);

    std::string rootCertPath = certRootPath + "/root_cert.pem";
    std::string clientCertPath = certRootPath + "/client_cert.pem";
    std::string clientKeyPath = certRootPath + "/client_key.pem";

    if (!std::filesystem::exists(rootCertPath)) {
        std::cerr << "Root certificate file not found: " << rootCertPath << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(clientCertPath)) {
        std::cerr << "Client certificate file not found: " << clientCertPath << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(clientKeyPath)) {
        std::cerr << "Client key file not found: " << clientKeyPath << std::endl;
        return 1;
    }

    std::string serverAddressWithPort = serverAddress + ":" + std::to_string(port);

    std::cerr << "Connecting to " << serverAddressWithPort << "..." << std::endl;
    OSLowLevelSdkClient client(serverAddressWithPort, rootCertPath, clientCertPath, clientKeyPath, controllerId);

    if (demoOffsets) {
        runOffsetDemo(client, offsetAmplitudeArcsec, offsetIntervalMicrosec);
    }
    else {
        runStreamingDemo(client, minIntervalMicrosec, timeoutMillisec);
    }    

    return 0;
}
