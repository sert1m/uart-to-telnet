/**
 * @file LinuxWiFiStub.h
 * @brief Linux stub implementation of WiFiModule.
 */

#ifndef PLATFORM_LINUX_WIFI_STUB_H
#define PLATFORM_LINUX_WIFI_STUB_H

#include "interfaces/WiFiModule.h"

/**
 * @brief Linux WiFi stub that reports always-connected status.
 *
 * On Linux, WiFi is managed by the OS. This stub always reports connected
 * and returns the system's current IP address (or "127.0.0.1" as fallback).
 */
class LinuxWiFiStub : public WiFiModule {
public:
    bool connect(const WiFiConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string getIpAddress() const override;
    int getSignalStrengthDbm() const override;
};

#endif // PLATFORM_LINUX_WIFI_STUB_H
