/**
 * @file MockHttpServer.h
 * @brief Google Mock implementation of HttpServer for unit testing.
 *
 * Allows tests to set expectations and verify interactions with the
 * HTTP server without requiring real network connections.
 */

#ifndef TEST_MOCKS_MOCK_HTTP_SERVER_H
#define TEST_MOCKS_MOCK_HTTP_SERVER_H

#include <gmock/gmock.h>
#include "interfaces/HttpServer.h"

class MockHttpServer : public HttpServer {
public:
    MOCK_METHOD(bool, start, (uint16_t), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, registerHandler, (const std::string&, const std::string&, std::function<HttpResponse(const std::string&)>), (override));
};

#endif // TEST_MOCKS_MOCK_HTTP_SERVER_H
