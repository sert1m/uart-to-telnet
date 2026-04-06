/**
 * @file Esp32UartModule.h
 * @brief ESP32 implementation of UartModule using ESP-IDF UART driver.
 */

#ifndef PLATFORM_ESP32_UART_MODULE_H
#define PLATFORM_ESP32_UART_MODULE_H

#include "interfaces/UartModule.h"

#include <functional>
#include <string>

/**
 * @brief ESP32 UART implementation using ESP-IDF UART driver.
 *
 * Uses the ESP32 hardware UART peripheral via the ESP-IDF uart driver API.
 * Configures baud rate, data bits, parity, and stop bits through
 * uart_param_config() and installs the UART driver with uart_driver_install().
 */
class Esp32UartModule : public UartModule {
public:
    Esp32UartModule();
    ~Esp32UartModule() override;

    bool open(const UartConfig& config) override;
    void close() override;
    int write(const uint8_t* data, size_t length) override;
    void setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) override;
    bool isOpen() const override;
    bool reopen() override;

private:
    UartConfig config_;
    bool opened_ = false;
    std::function<void(const uint8_t*, size_t)> onDataCallback_;

    // TODO: Add ESP32 UART port number (uart_port_t)
    // TODO: Add task handle for the UART read task
};

#endif // PLATFORM_ESP32_UART_MODULE_H
