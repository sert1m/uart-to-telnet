/**
 * @file LinuxDisplayModule.cpp
 * @brief Linux display implementation (console output).
 */

#include "LinuxDisplayModule.h"

#include <cstdio>

void LinuxDisplayModule::init() {
    std::printf("[Display] Linux console display initialized\n");
}

void LinuxDisplayModule::update(const DisplayStatus& status) {
    std::printf("[Display] IP: %s | UART RX: %u B/s | TX: %u B/s\n",
                status.ipAddress.c_str(),
                status.uartRxBytesPerSec,
                status.uartTxBytesPerSec);
}

void LinuxDisplayModule::setBacklight(bool /*on*/) {
    // No-op on Linux
}

void LinuxDisplayModule::onButtonPress() {
    // No-op on Linux
}
