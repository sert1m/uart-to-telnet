/**
 * @file M5HttpServer.h
 * @brief M5Stack no-op HTTP server stub (HTTP disabled for now).
 */

#ifndef PLATFORM_M5STACK_HTTP_SERVER_H
#define PLATFORM_M5STACK_HTTP_SERVER_H

#include "interfaces/HttpServer.h"

/**
 * @brief No-op HTTP server — HTTP is disabled on M5Stack for now.
 */
class M5HttpServer : public HttpServer {
public:
    bool start(uint16_t) override { return true; }
    void stop() override {}
    void registerHandler(const std::string&, const std::string&,
        std::function<HttpResponse(const std::string&)>) override {}
    void poll() {}
};

#endif // PLATFORM_M5STACK_HTTP_SERVER_H
