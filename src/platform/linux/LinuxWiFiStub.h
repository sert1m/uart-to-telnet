/**
 * @file LinuxWiFiStub.h
 * @brief Linux stub implementation of WiFiModule.
 */

#ifndef PLATFORM_LINUX_WIFI_STUB_H
#define PLATFORM_LINUX_WIFI_STUB_H

#include "interfaces/WiFiModule.h"
#include <string>

/**
 * @brief Linux WiFi stub that reports always-connected status.
 *
 * On Linux the host OS manages networking. This stub always reports
 * connected and resolves the IP address via getifaddrs(3):
 *   - If an interface name is configured, returns that interface's IPv4 address.
 *   - Otherwise, returns the first non-loopback IPv4 address found.
 *   - Falls back to "127.0.0.1" if nothing is found.
 */
class LinuxWiFiStub : public WiFiModule {
public:
    /**
     * @brief Construct with an optional preferred interface name.
     * @param iface Interface name, e.g. "eth0", "wlan0". Empty = auto-detect.
     */
    explicit LinuxWiFiStub(std::string iface = {});

    bool connect(const WiFiConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string getIpAddress() const override;
    int getSignalStrengthDbm() const override;

private:
    std::string iface_;  ///< Preferred interface name (empty = auto-detect).
};

#endif // PLATFORM_LINUX_WIFI_STUB_H
