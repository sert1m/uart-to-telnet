/**
 * @file Esp32TelnetServer.cpp
 * @brief ESP32 implementation of TelnetServer using lwIP sockets.
 *
 * Skeleton implementation with TODO comments for actual ESP32 lwIP/socket calls.
 * This file is NOT compiled on Linux — it is only included when PLATFORM=esp32.
 */

#include "Esp32TelnetServer.h"

// TODO: #include "lwip/sockets.h"
// TODO: #include "esp_log.h"

Esp32TelnetServer::Esp32TelnetServer() = default;

Esp32TelnetServer::~Esp32TelnetServer() {
    stop();
}

bool Esp32TelnetServer::start(uint16_t port) {
    // TODO: Create a TCP socket via lwip_socket(AF_INET, SOCK_STREAM, 0)
    // TODO: Set SO_REUSEADDR socket option
    // TODO: Bind to the specified port via lwip_bind()
    // TODO: Listen for connections via lwip_listen()
    // TODO: Create a FreeRTOS task for the accept loop:
    //   - Accept incoming connections via lwip_accept()
    //   - For each client, create a per-client read task
    //   - Invoke onClientEventCallback_(clientId, true) on connect
    //   - Invoke onClientDataCallback_ when data is received
    //   - Invoke onClientEventCallback_(clientId, false) on disconnect

    return false; // Skeleton: not implemented
}

void Esp32TelnetServer::stop() {
    // TODO: Close all client sockets
    // TODO: Close the server socket
    // TODO: Delete the accept task and per-client tasks
    // TODO: Reset clientCount_ to 0

    clientCount_ = 0;
}

void Esp32TelnetServer::broadcast(const uint8_t* data, size_t length) {
    // TODO: Iterate over all connected client sockets
    // TODO: Call lwip_send() for each client
    // TODO: Handle send errors (remove disconnected clients)
}

void Esp32TelnetServer::setOnClientDataCallback(
    std::function<void(int clientId, const uint8_t*, size_t)> callback) {
    onClientDataCallback_ = callback;
}

void Esp32TelnetServer::setOnClientEventCallback(
    std::function<void(int clientId, bool connected)> callback) {
    onClientEventCallback_ = callback;
}

int Esp32TelnetServer::clientCount() const {
    return clientCount_;
}
