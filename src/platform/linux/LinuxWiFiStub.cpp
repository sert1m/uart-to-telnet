/**
 * @file LinuxWiFiStub.cpp
 * @brief Linux WiFi stub — resolves IP via getifaddrs instead of hostname lookup.
 */

#include "LinuxWiFiStub.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <cstring>

LinuxWiFiStub::LinuxWiFiStub(std::string iface)
    : iface_(std::move(iface)) {}

bool LinuxWiFiStub::connect(const WiFiConfig& /*config*/) {
    return true;  // host OS manages networking
}

void LinuxWiFiStub::disconnect() {
    // No-op on Linux
}

bool LinuxWiFiStub::isConnected() const {
    return true;
}

std::string LinuxWiFiStub::getIpAddress() const {
    struct ifaddrs* ifap = nullptr;
    if (getifaddrs(&ifap) != 0 || ifap == nullptr) {
        return "127.0.0.1";
    }

    std::string result;
    char ipStr[INET_ADDRSTRLEN];

    for (struct ifaddrs* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        const bool isLoopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
        const bool isUp       = (ifa->ifa_flags & IFF_UP) != 0;
        if (!isUp) continue;

        auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof(ipStr));

        if (!iface_.empty()) {
            // Caller specified a preferred interface — use it if found
            if (std::strcmp(ifa->ifa_name, iface_.c_str()) == 0) {
                result = ipStr;
                break;
            }
        } else {
            // Auto-detect: take the first non-loopback UP interface
            if (!isLoopback) {
                result = ipStr;
                break;
            }
        }
    }

    freeifaddrs(ifap);
    return result.empty() ? "127.0.0.1" : result;
}

int LinuxWiFiStub::getSignalStrengthDbm() const {
    return 0;
}
