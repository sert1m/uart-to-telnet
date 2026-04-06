/**
 * @file LinuxHttpServer.h
 * @brief Linux implementation of HttpServer using POSIX sockets.
 */

#ifndef PLATFORM_LINUX_HTTP_SERVER_H
#define PLATFORM_LINUX_HTTP_SERVER_H

#include "interfaces/HttpServer.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

/**
 * @brief Linux HTTP server using POSIX TCP sockets.
 *
 * Creates a TCP server socket, accepts connections in a background thread,
 * parses minimal HTTP requests (method, path, body), and dispatches to
 * registered handlers.
 */
class LinuxHttpServer : public HttpServer {
public:
    LinuxHttpServer();
    ~LinuxHttpServer() override;

    bool start(uint16_t port) override;
    void stop() override;
    void registerHandler(
        const std::string& method,
        const std::string& path,
        std::function<HttpResponse(const std::string& body)> handler) override;

private:
    void acceptLoop();
    void handleClient(int clientFd);

    int serverFd_ = -1;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;

    std::mutex handlersMutex_;
    /// Key: "METHOD /path", Value: handler function
    std::map<std::string, std::function<HttpResponse(const std::string&)>> handlers_;
};

#endif // PLATFORM_LINUX_HTTP_SERVER_H
