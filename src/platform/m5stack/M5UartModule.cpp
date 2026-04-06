/**
 * @file M5UartModule.cpp
 * @brief M5Stack Arduino UART implementation using HardwareSerial (UART2).
 */

#include "M5UartModule.h"

M5UartModule::M5UartModule() = default;

bool M5UartModule::open(const UartConfig& config) {
    config_ = config;

    uint32_t serialConfig = SERIAL_8N1;

    // Map data bits + parity + stop bits to Arduino serial config
    uint8_t db = config.dataBits;
    bool even = (config.parity == "even");
    bool odd = (config.parity == "odd");
    uint8_t sb = config.stopBits;

    if (db == 8 && !even && !odd && sb == 1) serialConfig = SERIAL_8N1;
    else if (db == 8 && !even && !odd && sb == 2) serialConfig = SERIAL_8N2;
    else if (db == 8 && even && sb == 1) serialConfig = SERIAL_8E1;
    else if (db == 8 && even && sb == 2) serialConfig = SERIAL_8E2;
    else if (db == 8 && odd && sb == 1) serialConfig = SERIAL_8O1;
    else if (db == 8 && odd && sb == 2) serialConfig = SERIAL_8O2;
    else if (db == 7 && !even && !odd && sb == 1) serialConfig = SERIAL_7N1;
    else if (db == 7 && even && sb == 1) serialConfig = SERIAL_7E1;
    else if (db == 7 && odd && sb == 1) serialConfig = SERIAL_7O1;

    serial_.begin(config.baudRate, serialConfig, RX_PIN, TX_PIN);
    opened_ = true;
    return true;
}

void M5UartModule::close() {
    if (opened_) {
        serial_.end();
        opened_ = false;
    }
}

int M5UartModule::write(const uint8_t* data, size_t length) {
    if (!opened_) return -1;
    return static_cast<int>(serial_.write(data, length));
}

void M5UartModule::setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) {
    onDataCallback_ = std::move(callback);
}

bool M5UartModule::isOpen() const {
    return opened_;
}

bool M5UartModule::reopen() {
    close();
    return open(config_);
}

void M5UartModule::poll() {
    if (!opened_ || !onDataCallback_) return;

    uint8_t buf[256];
    size_t available = serial_.available();
    if (available > 0) {
        size_t toRead = (available > sizeof(buf)) ? sizeof(buf) : available;
        size_t n = serial_.readBytes(buf, toRead);
        if (n > 0) {
            onDataCallback_(buf, n);
        }
    }
}
