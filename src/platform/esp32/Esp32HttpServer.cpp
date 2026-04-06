/**
 * @file Esp32HttpServer.cpp
 * @brief ESP32 implementation of HttpServer using ESP-IDF HTTP server.
 *
 * Skeleton implementation with TODO comments for actual ESP32 HTTP server calls.
 * This file is NOT compiled on Linux — it is only included when PLATFORM=esp32.
 */

#include "Esp32HttpServer.h"

// TODO: #include "esp_http_server.h"
// TODO: #include "esp_log.h"

Esp32HttpServer::Esp32HttpServer() = default;

Esp32HttpServer::~Esp32HttpServer() {
    stop();
}

bool Esp32HttpServer::start(uint16_t port) {
    // TODO: Create httpd_config_t with the specified port
    //   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    //   config.server_port = port;
    // TODO: Start the HTTP server via httpd_start(&serverHandle_, &config)
    // TODO: Register all stored handlers as URI handlers via httpd_register_uri_handler()
    // TODO: Return true on success

    return false; // Skeleton: not implemented
}

void Esp32HttpServer::stop() {
    // TODO: Stop the HTTP server via httpd_stop(serverHandle_)
    // TODO: Set serverHandle_ to nullptr
}

void Esp32HttpServer::registerHandler(
    const std::string& method,
    const std::string& path,
    std::function<HttpResponse(const std::string& body)> handler) {
    // Store the handler for later registration when start() is called
    std::string key = method + ":" + path;
    handlers_[key] = handler;

    // TODO: If server is already running, register the URI handler immediately
    // TODO: Create httpd_uri_t with:
    //   - .uri = path.c_str()
    //   - .method = map method string to httpd_method_t (HTTP_GET, HTTP_POST, etc.)
    //   - .handler = wrapper function that reads request body, calls handler, sends response
    //   - .user_ctx = pointer to the stored handler
}
