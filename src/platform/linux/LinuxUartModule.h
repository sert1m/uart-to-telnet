/**
 * @file LinuxUartModule.h
 * @brief Linux implementation of UartModule using POSIX termios.
 */

#ifndef PLATFORM_LINUX_UART_MODULE_H
#define PLATFORM_LINUX_UART_MODULE_H

#include "interfaces/UartModule.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

/**
 * @brief Linux UART implementation using POSIX termios and file descriptors.
 *
 * Opens a serial port, configures it with termios, and spawns a background
 * read thread that delivers received data via the registered callback.
 */
class LinuxUartModule : public UartModule {
public:
    LinuxUartModule();
    ~LinuxUartModule() override;

    bool open(const UartConfig& config) override;
    void close() override;
    int write(const uint8_t* data, size_t length) override;
    void setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) override;
    bool isOpen() const override;
    bool reopen() override;

private:
    void readLoop();

    int fd_ = -1;
    UartConfig config_;
    std::atomic<bool> running_{false};
    std::thread readThread_;
    std::function<void(const uint8_t*, size_t)> onDataCallback_;
};

#endif // PLATFORM_LINUX_UART_MODULE_H
