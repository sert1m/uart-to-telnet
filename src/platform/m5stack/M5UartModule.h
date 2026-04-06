/**
 * @file M5UartModule.h
 * @brief M5Stack Arduino implementation of UartModule using HardwareSerial.
 */

#ifndef PLATFORM_M5STACK_UART_MODULE_H
#define PLATFORM_M5STACK_UART_MODULE_H

#include "interfaces/UartModule.h"
#include <HardwareSerial.h>

/**
 * @brief M5Stack UART using HardwareSerial (UART2) on GPIO 16 (RX) / 17 (TX).
 */
class M5UartModule : public UartModule {
public:
    M5UartModule();
    ~M5UartModule() override = default;

    bool open(const UartConfig& config) override;
    void close() override;
    int write(const uint8_t* data, size_t length) override;
    void setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) override;
    bool isOpen() const override;
    bool reopen() override;

    /// Call from loop() to poll for incoming serial data.
    void poll();

private:
    static constexpr int RX_PIN = 16;
    static constexpr int TX_PIN = 17;

    HardwareSerial serial_{2}; // UART2
    UartConfig config_;
    bool opened_ = false;
    std::function<void(const uint8_t*, size_t)> onDataCallback_;
};

#endif // PLATFORM_M5STACK_UART_MODULE_H
