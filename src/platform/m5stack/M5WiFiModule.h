/**
 * @file M5WiFiModule.h
 * @brief M5Stack Arduino WiFi implementation using WiFi.h.
 */

#ifndef PLATFORM_M5STACK_WIFI_MODULE_H
#define PLATFORM_M5STACK_WIFI_MODULE_H

#include "interfaces/WiFiModule.h"
#include <WiFi.h>

/**
 * @brief M5Stack WiFi using Arduino WiFi.h with retry logic.
 */
class M5WiFiModule : public WiFiModule {
public:
    bool connect(const WiFiConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string getIpAddress() const override;
    int getSignalStrengthDbm() const override;

private:
    WiFiConfig config_;
};

#endif // PLATFORM_M5STACK_WIFI_MODULE_H
