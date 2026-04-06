/**
 * @file Esp32WiFiModule.h
 * @brief ESP32 implementation of WiFiModule using ESP-IDF WiFi API.
 */

#ifndef PLATFORM_ESP32_WIFI_MODULE_H
#define PLATFORM_ESP32_WIFI_MODULE_H

#include "interfaces/WiFiModule.h"

#include <string>

/**
 * @brief ESP32 WiFi implementation using ESP-IDF WiFi station mode.
 *
 * Connects to a preconfigured WiFi access point with retry logic:
 * - Retries at 5-second intervals up to maxRetries
 * - Restarts the connection cycle if maxRetries is exceeded
 * - Auto-reconnects on connection drop
 */
class Esp32WiFiModule : public WiFiModule {
public:
    Esp32WiFiModule();
    ~Esp32WiFiModule() override;

    bool connect(const WiFiConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string getIpAddress() const override;
    int getSignalStrengthDbm() const override;

private:
    bool connected_ = false;
    std::string ipAddress_;
    WiFiConfig config_;

    // TODO: WiFi retry logic structure:
    //   - Retry interval: 5 seconds between attempts
    //   - Max retries from config.maxRetries
    //   - On maxRetries exceeded: log error, restart connection cycle
    //   - Auto-reconnect: register WiFi event handler for WIFI_EVENT_STA_DISCONNECTED
    //     that triggers reconnection automatically

    // TODO: Add ESP-IDF WiFi event group handle (EventGroupHandle_t)
    // TODO: Add event handler instance for WiFi and IP events
};

#endif // PLATFORM_ESP32_WIFI_MODULE_H
