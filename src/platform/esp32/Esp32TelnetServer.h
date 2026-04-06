/**
 * @file Esp32TelnetServer.h
 * @brief ESP32 implementation of TelnetServer using lwIP sockets.
 */

#ifndef PLATFORM_ESP32_TELNET_SERVER_H
#define PLATFORM_ESP32_TELNET_SERVER_H

#include "interfaces/TelnetServer.h"

#include <functional>

/**
 * @brief ESP32 Telnet server implementation using lwIP TCP sockets.
 *
 * Listens for incoming TCP connections on the configured port and manages
 * multiple simultaneous Telnet client sessions. Uses FreeRTOS tasks for
 * non-blocking accept and per-client read loops.
 */
class Esp32TelnetServer : public TelnetServer {
public:
    Esp32TelnetServer();
    ~Esp32TelnetServer() override;

    bool start(uint16_t port) override;
    void stop() override;
    void broadcast(const uint8_t* data, size_t length) override;
    void setOnClientDataCallback(
        std::function<void(int clientId, const uint8_t*, size_t)> callback) override;
    void setOnClientEventCallback(
        std::function<void(int clientId, bool connected)> callback) override;
    int clientCount() const override;

private:
    std::function<void(int, const uint8_t*, size_t)> onClientDataCallback_;
    std::function<void(int, bool)> onClientEventCallback_;
    int clientCount_ = 0;

    // TODO: Add lwIP server socket file descriptor
    // TODO: Add client socket list/map
    // TODO: Add FreeRTOS task handle for accept loop
};

#endif // PLATFORM_ESP32_TELNET_SERVER_H
