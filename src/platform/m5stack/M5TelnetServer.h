/**
 * @file M5TelnetServer.h
 * @brief M5Stack Arduino implementation of TelnetServer using WiFiServer.
 */

#ifndef PLATFORM_M5STACK_TELNET_SERVER_H
#define PLATFORM_M5STACK_TELNET_SERVER_H

#include "interfaces/TelnetServer.h"
#include <WiFi.h>
#include <vector>

static constexpr int MAX_TELNET_CLIENTS = 4;

/**
 * @brief M5Stack Telnet server using Arduino WiFiServer/WiFiClient.
 */
class M5TelnetServer : public TelnetServer {
public:
    M5TelnetServer();
    ~M5TelnetServer() override = default;

    bool start(uint16_t port) override;
    void stop() override;
    void broadcast(const uint8_t* data, size_t length) override;
    void setOnClientDataCallback(
        std::function<void(int clientId, const uint8_t*, size_t)> callback) override;
    void setOnClientEventCallback(
        std::function<void(int clientId, bool connected)> callback) override;
    int clientCount() const override;

    /// Call from loop() to accept new clients and read data.
    void poll();

private:
    WiFiServer* server_ = nullptr;
    uint16_t port_ = 0;
    bool running_ = false;
    int nextClientId_ = 1;

    struct ClientSlot {
        WiFiClient client;
        int id = 0;
        bool active = false;
    };
    ClientSlot clients_[MAX_TELNET_CLIENTS];

    std::function<void(int, const uint8_t*, size_t)> onClientData_;
    std::function<void(int, bool)> onClientEvent_;
};

#endif // PLATFORM_M5STACK_TELNET_SERVER_H
