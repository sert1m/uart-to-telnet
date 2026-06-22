/**
 * @file LinuxTelnetServer.cpp
 * @brief Linux Telnet server implementation using POSIX sockets.
 */

#include "LinuxTelnetServer.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

LinuxTelnetServer::LinuxTelnetServer() = default;

LinuxTelnetServer::~LinuxTelnetServer() {
    stop();
}

bool LinuxTelnetServer::start(uint16_t port) {
    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        return false;
    }

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(serverFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    if (::listen(serverFd_, 5) < 0) {
        ::close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    running_ = true;
    acceptThread_ = std::thread(&LinuxTelnetServer::acceptLoop, this);
    return true;
}

void LinuxTelnetServer::stop() {
    running_ = false;

    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (int fd : clientFds_) {
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
        clientFds_.clear();
    }

    for (auto& t : clientThreads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    clientThreads_.clear();
}

void LinuxTelnetServer::broadcast(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (int fd : clientFds_) {
        ::send(fd, data, length, MSG_NOSIGNAL);
    }
}

void LinuxTelnetServer::setOnClientDataCallback(
    std::function<void(int clientId, const uint8_t*, size_t)> callback) {
    onClientData_ = std::move(callback);
}

void LinuxTelnetServer::setOnClientEventCallback(
    std::function<void(int clientId, bool connected)> callback) {
    onClientEvent_ = std::move(callback);
}

int LinuxTelnetServer::clientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return static_cast<int>(clientFds_.size());
}

void LinuxTelnetServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd_,
                                reinterpret_cast<struct sockaddr*>(&clientAddr),
                                &addrLen);
        if (clientFd < 0) {
            continue;
        }

        int clientId = nextClientId_++;
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clientFds_.push_back(clientFd);
        }

        if (onClientEvent_) {
            onClientEvent_(clientId, true);
        }

        clientThreads_.emplace_back(&LinuxTelnetServer::clientLoop, this, clientFd, clientId);
    }
}

void LinuxTelnetServer::clientLoop(int clientFd, int clientId) {
    // Send IAC DONT for any option the client might offer, suppressing all
    // telnet negotiations so they don't get forwarded to the UART as garbage.
    // Also send IAC WILL SUPPRESS-GO-AHEAD to satisfy basic telnet clients.
    static const uint8_t initOpts[] = {
        0xFF, 0xFB, 0x03,  // IAC WILL SUPPRESS-GO-AHEAD
        0xFF, 0xFB, 0x01,  // IAC WILL ECHO
    };
    ::send(clientFd, initOpts, sizeof(initOpts), MSG_NOSIGNAL);

    uint8_t buf[1024];
    // Small state machine to strip IAC sequences from the incoming stream
    // before forwarding to UART.
    //   state 0: normal data
    //   state 1: seen IAC (0xFF), waiting for command byte
    //   state 2: seen IAC + DO/DONT/WILL/WONT, waiting for option byte
    int iacState = 0;
    uint8_t iacCmd = 0;

    while (running_) {
        ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }

        // Filter out IAC sequences; pass clean bytes to callback
        uint8_t clean[1024];
        size_t cleanLen = 0;

        for (ssize_t i = 0; i < n; ++i) {
            uint8_t b = buf[i];
            switch (iacState) {
                case 0:
                    if (b == 0xFF) {          // IAC
                        iacState = 1;
                    } else {
                        clean[cleanLen++] = b;
                    }
                    break;
                case 1:
                    iacCmd = b;
                    if (b == 0xFB || b == 0xFC || b == 0xFD || b == 0xFE) {
                        // WILL / WONT / DO / DONT — one option byte follows
                        iacState = 2;
                    } else if (b == 0xFF) {
                        // IAC IAC = literal 0xFF
                        clean[cleanLen++] = 0xFF;
                        iacState = 0;
                    } else {
                        // 2-byte command (e.g. IAC NOP, IAC GA) — done
                        iacState = 0;
                    }
                    break;
                case 2:
                    // Option byte — respond: refuse WILL→DONT, DO→WONT
                    {
                        uint8_t reply[3];
                        reply[0] = 0xFF; // IAC
                        if (iacCmd == 0xFB) {       // WILL → DONT
                            reply[1] = 0xFE; reply[2] = b;
                            ::send(clientFd, reply, 3, MSG_NOSIGNAL);
                        } else if (iacCmd == 0xFD) { // DO → WONT
                            reply[1] = 0xFC; reply[2] = b;
                            ::send(clientFd, reply, 3, MSG_NOSIGNAL);
                        }
                        // WONT / DONT — no reply needed
                    }
                    iacState = 0;
                    break;
            }
        }

        if (cleanLen > 0 && onClientData_) {
            onClientData_(clientId, clean, cleanLen);
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clientFds_.erase(
            std::remove(clientFds_.begin(), clientFds_.end(), clientFd),
            clientFds_.end());
    }
    ::close(clientFd);

    if (onClientEvent_) {
        onClientEvent_(clientId, false);
    }
}
