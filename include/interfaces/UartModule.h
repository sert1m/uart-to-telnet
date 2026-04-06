/**
 * @file UartModule.h
 * @brief Abstract interface for UART communication.
 *
 * Provides a platform-independent contract for opening, reading from,
 * and writing to a hardware UART port. Platform-specific implementations
 * (e.g., Esp32UartModule, LinuxUartModule) inherit from this class.
 */

#ifndef INTERFACES_UART_MODULE_H
#define INTERFACES_UART_MODULE_H

#include "core/DataModels.h"

#include <cstdint>
#include <functional>

/**
 * @brief Abstract interface for UART communication.
 *
 * Provides a platform-independent contract for opening, reading from,
 * and writing to a hardware UART port. Platform-specific implementations
 * (e.g., Esp32UartModule, LinuxUartModule) inherit from this class.
 */
class UartModule {
public:
    virtual ~UartModule() = default;

    /**
     * @brief Open the UART port with the given configuration.
     * @param config UART configuration (port name, baud rate, data bits, parity, stop bits).
     * @return true if the port was opened successfully, false on error.
     * @pre @p config contains valid portName, baudRate, dataBits, parity, stopBits.
     * @post Port is open and ready for read/write, or an error is reported.
     */
    virtual bool open(const UartConfig& config) = 0;

    /**
     * @brief Close the UART port and release associated resources.
     */
    virtual void close() = 0;

    /**
     * @brief Write bytes to the UART port.
     * @param data Pointer to the byte buffer to transmit.
     * @param length Number of bytes to write.
     * @return Number of bytes written, or -1 on error.
     * @pre Port is open (isOpen() returns true).
     * @note Bytes are sent in the order provided.
     */
    virtual int write(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Register a callback invoked when data is received from UART.
     * @param callback Function receiving a pointer to received data and its length.
     * @note Data must be delivered to the callback within 50ms of hardware reception.
     */
    virtual void setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) = 0;

    /**
     * @brief Check whether the port is currently open and operational.
     * @return true if the port is open, false otherwise.
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief Attempt to reopen the port after a connection loss.
     * @return true if the port was reopened successfully, false on error.
     */
    virtual bool reopen() = 0;
};

#endif // INTERFACES_UART_MODULE_H
