/**
 * @file LinuxTelnetServer.h
 * @brief Linux implementation of TelnetServer using POSIX sockets.
 */

#ifndef PLATFORM_LINUX_TELNET_SERVER_H
#define PLATFORM_LINUX_TELNET_SERVER_H

#include "interfaces/TelnetServer.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief Linux Telnet server using POSIX TCP sockets.
 *
 * Creates a TCP server socket, accepts client connections in a background
 * thread, and spawns per-client read threads. Broadcasts data to all
 * connected clients.
 */
class LinuxTelnetServer : public TelnetServer {
public:
    LinuxTelnetServer();
    ~LinuxTelnetServer() override;

    bool start(uint16_t port) override;
    void stop() override;
    void broadcast(const uint8_t* data, size_t length) override;
    void setOnClientDataCallback(
        std::function<void(int clientId, const uint8_t*, size_t)> callback) override;
    void setOnClientEventCallback(
        std::function<void(int clientId, bool connected)> callback) override;
    int clientCount() const override;

private:
    void acceptLoop();
    void clientLoop(int clientFd, int clientId);

    int serverFd_ = -1;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;

    mutable std::mutex clientsMutex_;
    std::vector<int> clientFds_;
    std::vector<std::thread> clientThreads_;
    int nextClientId_ = 1;

    std::function<void(int, const uint8_t*, size_t)> onClientData_;
    std::function<void(int, bool)> onClientEvent_;
};

#endif // PLATFORM_LINUX_TELNET_SERVER_H
