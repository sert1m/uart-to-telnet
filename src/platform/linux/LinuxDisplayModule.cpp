/**
 * @file LinuxDisplayModule.cpp
 * @brief Linux display implementation: console output + /tmp status file.
 */

#include "LinuxDisplayModule.h"
#include "Version.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>

LinuxDisplayModule::LinuxDisplayModule(const std::string& statusFile)
    : statusFile_(statusFile) {}

void LinuxDisplayModule::init() {
    std::cout << "[Display] UART-Telnet Bridge v" << SW_VERSION
              << " — status file: " << statusFile_ << std::endl;

    // Write initial status file so it exists from startup
    std::ofstream f(statusFile_, std::ios::trunc);
    if (f.is_open()) {
        f << "UART-Telnet Bridge v" << SW_VERSION << "\n";
        f << "Status: starting...\n";
    }
}

void LinuxDisplayModule::update(const DisplayStatus& status) {
    ++uptimeSec_;

    // --- Console output (brief) ---
    std::printf("[Display] IP: %s | RX: %u B/s | TX: %u B/s | Up: %us\n",
                status.ipAddress.c_str(),
                status.uartRxBytesPerSec,
                status.uartTxBytesPerSec,
                uptimeSec_);

    // --- Status file (detailed, mirrors M5Stack display) ---
    std::ofstream f(statusFile_, std::ios::trunc);
    if (!f.is_open()) return;

    // Timestamp
    std::time_t now = std::time(nullptr);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    f << "=== UART-Telnet Bridge v" << SW_VERSION << " ===\n";
    f << "Updated : " << timeBuf << "\n";
    f << "\n";
    f << "Network\n";
    f << "  IP      : " << status.ipAddress << "\n";
    f << "\n";
    f << "UART Throughput\n";
    f << "  RX      : " << status.uartRxBytesPerSec << " B/s\n";
    f << "  TX      : " << status.uartTxBytesPerSec << " B/s\n";
    f << "\n";
    f << "Uptime    : " << uptimeSec_ << " s\n";
    f << "\n";

    if (storage_) {
        if (storage_->isAvailable()) {
            f << "Log Files : " << storage_->getFileCount()
              << " file(s), current: " << storage_->getCurrentFileName() << "\n";
        } else {
            f << "Log Files : unavailable\n";
        }
    } else {
        f << "Log Files : disabled\n";
    }
}

void LinuxDisplayModule::setBacklight(bool /*on*/) {
    // No-op on Linux
}

void LinuxDisplayModule::onButtonPress() {
    // No-op on Linux
}
