#include "client.hpp"
#include <iostream>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <CLI/CLI.hpp>

int main(int argc, char *argv[]) {
    CLI::App app{"gRPC Client for Observatory Status"};
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
    
    std::string nodeControllerId;
    app.add_option("--controller-id", nodeControllerId, 
        "The UUID of the node controller.\n"
        "TIP: You can read this from your certificate file as follows:\n"
        "openssl x509 -in client_cert.pem -noout -text | grep nodeControllerId\n"
    )->required();

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
    OSLowLevelSdkClient client(serverAddressWithPort, rootCertPath, clientCertPath, clientKeyPath, nodeControllerId);

    std::cout << "# NumReceivedMessages, DC Time (usec since 1970), Received time (usec since 1970), Timestamp Age (usec), Timestamp Interval (msec)\n";

    double lastMessageTimeSeconds = 0;
    int messageCount = 0;
    client.StreamObservatoryStatus(minIntervalMicrosec, timeoutMillisec, [&lastMessageTimeSeconds, &messageCount](oslowlevelsdk::V1ObservatoryStatus status) {
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
