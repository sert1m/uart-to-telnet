/**
 * @file LinuxWiFiStub.cpp
 * @brief Linux WiFi stub implementation.
 */

#include "LinuxWiFiStub.h"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool LinuxWiFiStub::connect(const WiFiConfig& /*config*/) {
    return true;
}

void LinuxWiFiStub::disconnect() {
    // No-op on Linux
}

bool LinuxWiFiStub::isConnected() const {
    return true;
}

std::string LinuxWiFiStub::getIpAddress() const {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "127.0.0.1";
    }

    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0 || result == nullptr) {
        return "127.0.0.1";
    }

    char ipStr[INET_ADDRSTRLEN];
    auto* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
    freeaddrinfo(result);

    return std::string(ipStr);
}

int LinuxWiFiStub::getSignalStrengthDbm() const {
    return 0;
}
