/**
 * @file Esp32UartModule.cpp
 * @brief ESP32 implementation of UartModule using ESP-IDF UART driver.
 *
 * Skeleton implementation with TODO comments for actual ESP32 SDK calls.
 * This file is NOT compiled on Linux — it is only included when PLATFORM=esp32.
 */

#include "Esp32UartModule.h"

// TODO: #include "driver/uart.h"
// TODO: #include "esp_log.h"

Esp32UartModule::Esp32UartModule() = default;

Esp32UartModule::~Esp32UartModule() {
    close();
}

bool Esp32UartModule::open(const UartConfig& config) {
    config_ = config;

    // TODO: Map config.portName to uart_port_t (e.g., UART_NUM_1, UART_NUM_2)
    // TODO: Configure UART parameters via uart_param_config():
    //   - baud_rate = config.baudRate
    //   - data_bits = map config.dataBits to uart_word_length_t
    //   - parity = map config.parity to uart_parity_t
    //   - stop_bits = map config.stopBits to uart_stop_bits_t
    //   - flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    // TODO: Set UART pins via uart_set_pin() (TX, RX, RTS, CTS)
    // TODO: Install UART driver via uart_driver_install() with RX/TX buffer sizes
    // TODO: Create a FreeRTOS task for reading UART data and invoking onDataCallback_
    // TODO: Set opened_ = true on success

    return false; // Skeleton: not implemented
}

void Esp32UartModule::close() {
    // TODO: Delete the UART read task if running
    // TODO: Call uart_driver_delete() to release the UART driver
    // TODO: Set opened_ = false

    opened_ = false;
}

int Esp32UartModule::write(const uint8_t* data, size_t length) {
    // TODO: Call uart_write_bytes(uartPort_, data, length)
    // TODO: Return the number of bytes written, or -1 on error

    return -1; // Skeleton: not implemented
}

void Esp32UartModule::setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) {
    onDataCallback_ = callback;
}

bool Esp32UartModule::isOpen() const {
    return opened_;
}

bool Esp32UartModule::reopen() {
    // TODO: Close the current UART connection and reopen with stored config_
    close();
    return open(config_);
}
