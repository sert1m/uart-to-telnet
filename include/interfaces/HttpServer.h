/**
 * @file HttpServer.h
 * @brief Abstract interface for the HTTP configuration server.
 *
 * Provides a platform-independent contract for serving HTTP requests
 * used to configure the AutoResponder at runtime.
 */

#ifndef INTERFACES_HTTP_SERVER_H
#define INTERFACES_HTTP_SERVER_H

#include "core/DataModels.h"

#include <cstdint>
#include <functional>
#include <string>

/**
 * @brief Abstract interface for the HTTP configuration server.
 *
 * Provides a platform-independent contract for serving HTTP requests
 * used to configure the AutoResponder at runtime.
 */
class HttpServer {
public:
    virtual ~HttpServer() = default;

    /**
     * @brief Start listening for incoming HTTP connections.
     * @param port TCP port number to listen on.
     * @return true if the server started successfully, false if the port is in use.
     */
    virtual bool start(uint16_t port) = 0;

    /**
     * @brief Stop the HTTP server and release resources.
     */
    virtual void stop() = 0;

    /**
     * @brief Register a handler for a specific HTTP method and path.
     * @param method HTTP method string (e.g., "GET", "POST").
     * @param path URL path to handle (e.g., "/config").
     * @param handler Function that receives the request body and returns an HttpResponse.
     */
    virtual void registerHandler(
        const std::string& method,
        const std::string& path,
        std::function<HttpResponse(const std::string& body)> handler) = 0;
};

#endif // INTERFACES_HTTP_SERVER_H
