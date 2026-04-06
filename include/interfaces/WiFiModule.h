/**
 * @file WiFiModule.h
 * @brief Abstract interface for WiFi connectivity.
 *
 * On M5 Stack (ESP32), connects to a preconfigured WiFi access point with
 * retry logic. On Linux, provides a stub that reports always-connected status
 * and returns the system's current IP address.
 */

#ifndef INTERFACES_WIFI_MODULE_H
#define INTERFACES_WIFI_MODULE_H

#include "core/DataModels.h"

#include <string>

/**
 * @brief Abstract interface for WiFi connectivity.
 *
 * On M5 Stack (ESP32), connects to a preconfigured WiFi access point with
 * retry logic. On Linux, provides a stub that reports always-connected status
 * and returns the system's current IP address.
 */
class WiFiModule {
public:
    virtual ~WiFiModule() = default;

    /**
     * @brief Connect to the configured WiFi access point.
     * @param config WiFi configuration (SSID, password, max retries).
     * @return true if connected successfully, false if retries exhausted.
     * @pre @p config contains non-empty ssid and password.
     * @post Connected to WiFi, or retries exhausted and error logged.
     */
    virtual bool connect(const WiFiConfig& config) = 0;

    /**
     * @brief Disconnect from the current access point.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check whether the module is currently connected to WiFi.
     * @return true if connected, false otherwise. Linux stub always returns true.
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Get the current IP address.
     * @return IP address as a string (e.g., "192.168.1.100"), or empty string if not connected.
     */
    virtual std::string getIpAddress() const = 0;

    /**
     * @brief Get the current WiFi signal strength.
     * @return Signal strength in dBm. Linux stub returns 0.
     */
    virtual int getSignalStrengthDbm() const = 0;
};

#endif // INTERFACES_WIFI_MODULE_H
