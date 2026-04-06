/**
 * @file LinuxHttpServer.cpp
 * @brief Linux HTTP server implementation using POSIX sockets.
 */

#include "LinuxHttpServer.h"

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

LinuxHttpServer::LinuxHttpServer() = default;

LinuxHttpServer::~LinuxHttpServer() {
    stop();
}

bool LinuxHttpServer::start(uint16_t port) {
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
    acceptThread_ = std::thread(&LinuxHttpServer::acceptLoop, this);
    return true;
}

void LinuxHttpServer::stop() {
    running_ = false;

    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
}

void LinuxHttpServer::registerHandler(
    const std::string& method,
    const std::string& path,
    std::function<HttpResponse(const std::string& body)> handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_[method + " " + path] = std::move(handler);
}

void LinuxHttpServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd_,
                                reinterpret_cast<struct sockaddr*>(&clientAddr),
                                &addrLen);
        if (clientFd < 0) {
            continue;
        }
        handleClient(clientFd);
        ::close(clientFd);
    }
}

void LinuxHttpServer::handleClient(int clientFd) {
    // Read the full request (up to 8KB)
    char buf[8192];
    ssize_t n = ::recv(clientFd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        return;
    }
    buf[n] = '\0';
    std::string request(buf, static_cast<size_t>(n));

    // Parse request line: "METHOD /path HTTP/1.x"
    std::istringstream stream(request);
    std::string method, path, version;
    stream >> method >> path >> version;

    // Extract body (after \r\n\r\n)
    std::string body;
    auto bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        body = request.substr(bodyPos + 4);
    }

    // Look up handler
    HttpResponse response;
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        auto it = handlers_.find(method + " " + path);
        if (it != handlers_.end()) {
            response = it->second(body);
        } else {
            response.statusCode = 404;
            response.body = R"({"error":"Not found"})";
            response.contentType = "application/json";
        }
    }

    // Build HTTP response
    std::string statusText;
    switch (response.statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        default:  statusText = "Unknown"; break;
    }

    std::ostringstream resp;
    resp << "HTTP/1.1 " << response.statusCode << " " << statusText << "\r\n"
         << "Content-Type: " << response.contentType << "\r\n"
         << "Content-Length: " << response.body.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n"
         << response.body;

    std::string respStr = resp.str();
    ::send(clientFd, respStr.c_str(), respStr.size(), MSG_NOSIGNAL);
}
