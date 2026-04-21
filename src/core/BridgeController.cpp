/**
 * @file BridgeController.cpp
 * @brief Implementation of the BridgeController orchestrator.
 *
 * @see BridgeController.h for class documentation.
 */

#include "core/BridgeController.h"
#include "core/ConfigParser.h"
#include "interfaces/DisplayModule.h"
#include "interfaces/HttpServer.h"
#include "interfaces/StorageModule.h"
#include "interfaces/TelnetServer.h"
#include "interfaces/UartModule.h"
#include "interfaces/WiFiModule.h"

#include <iostream>

BridgeController::BridgeController(UartModule& uart,
                                   TelnetServer& telnet,
                                   HttpServer& http,
                                   DisplayModule& display,
                                   WiFiModule& wifi,
                                   AutoResponder& autoResponder,
                                   StorageModule* storage)
    : uart_(uart)
    , telnet_(telnet)
    , http_(http)
    , display_(display)
    , wifi_(wifi)
    , autoResponder_(autoResponder)
    , storage_(storage) {}

bool BridgeController::startup(const BridgeConfig& config) {
    config_ = config;

    // 1. Initialize display
    display_.init();

    // 2. Connect WiFi
    if (!wifi_.connect(config_.wifi)) {
        std::cerr << "BridgeController: WiFi connection failed for SSID \""
                  << config_.wifi.ssid << "\"" << std::endl;
        return false;
    }

    // 3. Open UART
    if (!uart_.open(config_.uart)) {
        std::cerr << "BridgeController: failed to open UART port \""
                  << config_.uart.portName << "\"" << std::endl;
        wifi_.disconnect();
        return false;
    }

    // 4. Start Telnet server
    if (!telnet_.start(config_.telnetPort)) {
        std::cerr << "BridgeController: failed to start Telnet server on port "
                  << config_.telnetPort << std::endl;
        uart_.close();
        wifi_.disconnect();
        return false;
    }

    // 5. Start HTTP server
    if (!http_.start(config_.httpPort)) {
        std::cerr << "BridgeController: failed to start HTTP server on port "
                  << config_.httpPort << std::endl;
        telnet_.stop();
        uart_.close();
        wifi_.disconnect();
        return false;
    }

    // Wire callbacks and handlers
    wireUartCallback();
    wireTelnetCallback();
    registerHttpHandlers();

    // Configure AutoResponder with the initial command sequence
    autoResponder_.setConfig(config_.commandSequence);

    return true;
}

void BridgeController::shutdown() {
    // Close in reverse initialization order: HTTP, Telnet, UART, WiFi, display
    http_.stop();
    telnet_.stop();
    uart_.close();
    wifi_.disconnect();
    // Display has no close/stop — just leave it as-is
}

void BridgeController::tick(uint32_t elapsedMs) {
    // Forward tick to AutoResponder for timeout/delay handling
    autoResponder_.tick(elapsedMs);

    // Accumulate time for throughput calculation
    throughputElapsedMs_ += elapsedMs;

    if (throughputElapsedMs_ >= kThroughputIntervalMs) {
        updateThroughputAndDisplay();
    }
}

void BridgeController::onUartConnectionLost() {
    std::cerr << "BridgeController: UART connection lost, attempting reopen" << std::endl;
    uart_.reopen();
}

uint32_t BridgeController::getRxBytesPerSec() const {
    return rxBytesPerSec_;
}

uint32_t BridgeController::getTxBytesPerSec() const {
    return txBytesPerSec_;
}

void BridgeController::wireUartCallback() {
    uart_.setOnDataCallback([this](const uint8_t* data, size_t length) {
        // Track RX bytes
        rxByteCount_ += static_cast<uint32_t>(length);

        // Forward to AutoResponder for prompt matching
        autoResponder_.process(data, length);

        // Broadcast to all Telnet clients
        telnet_.broadcast(data, length);

        // Write to storage if available
        if (storage_) {
            storage_->write(data, length);
        }
    });
}

void BridgeController::wireTelnetCallback() {
    telnet_.setOnClientDataCallback([this](int /*clientId*/, const uint8_t* data, size_t length) {
        // Track TX bytes
        txByteCount_ += static_cast<uint32_t>(length);

        // Forward to UART
        uart_.write(data, length);
    });
}

void BridgeController::registerHttpHandlers() {
    // POST /config — update AutoResponder configuration
    http_.registerHandler("POST", "/config", [this](const std::string& body) -> HttpResponse {
        auto result = ConfigParser::parseCommandSequence(body);
        if (!result.second.empty()) {
            HttpResponse resp;
            resp.statusCode = 400;
            resp.body = "{\"error\":\"" + result.second + "\"}";
            resp.contentType = "application/json";
            return resp;
        }
        autoResponder_.setConfig(result.first);
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = "{\"status\":\"ok\"}";
        resp.contentType = "application/json";
        return resp;
    });

    // GET /config — return current AutoResponder configuration
    http_.registerHandler("GET", "/config", [this](const std::string& /*body*/) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = ConfigParser::serializeCommandSequence(autoResponder_.getConfig());
        resp.contentType = "application/json";
        return resp;
    });

    // Default 404 handler for unknown endpoints
    http_.registerHandler("*", "*", [](const std::string& /*body*/) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 404;
        resp.body = "{\"error\":\"Not found\"}";
        resp.contentType = "application/json";
        return resp;
    });
}

void BridgeController::updateThroughputAndDisplay() {
    float elapsedSec = static_cast<float>(throughputElapsedMs_) / 1000.0f;
    if (elapsedSec > 0.0f) {
        rxBytesPerSec_ = static_cast<uint32_t>(static_cast<float>(rxByteCount_) / elapsedSec + 0.5f);
        txBytesPerSec_ = static_cast<uint32_t>(static_cast<float>(txByteCount_) / elapsedSec + 0.5f);
    } else {
        rxBytesPerSec_ = 0;
        txBytesPerSec_ = 0;
    }

    // Reset counters
    rxByteCount_ = 0;
    txByteCount_ = 0;
    throughputElapsedMs_ = 0;

    // Build display status and update
    DisplayStatus status;
    status.wifiConnected = wifi_.isConnected();
    status.signalStrengthDbm = wifi_.getSignalStrengthDbm();
    status.ipAddress = wifi_.getIpAddress();
    status.uartRxBytesPerSec = rxBytesPerSec_;
    status.uartTxBytesPerSec = txBytesPerSec_;

    display_.update(status);
}
