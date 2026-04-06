/**
 * @file Esp32HttpServer.h
 * @brief ESP32 implementation of HttpServer using ESP-IDF HTTP server.
 */

#ifndef PLATFORM_ESP32_HTTP_SERVER_H
#define PLATFORM_ESP32_HTTP_SERVER_H

#include "interfaces/HttpServer.h"

#include <functional>
#include <map>
#include <string>

/**
 * @brief ESP32 HTTP server implementation using ESP-IDF httpd component.
 *
 * Uses the ESP-IDF esp_http_server API to register URI handlers and
 * serve HTTP requests for runtime configuration of the AutoResponder.
 */
class Esp32HttpServer : public HttpServer {
public:
    Esp32HttpServer();
    ~Esp32HttpServer() override;

    bool start(uint16_t port) override;
    void stop() override;
    void registerHandler(
        const std::string& method,
        const std::string& path,
        std::function<HttpResponse(const std::string& body)> handler) override;

private:
    // TODO: Add httpd_handle_t server handle
    // TODO: Store registered handlers for URI dispatch
    std::map<std::string, std::function<HttpResponse(const std::string&)>> handlers_;
};

#endif // PLATFORM_ESP32_HTTP_SERVER_H
