/**
 * @file MockTelnetServer.h
 * @brief Google Mock implementation of TelnetServer for unit testing.
 *
 * Allows tests to set expectations and verify interactions with the
 * Telnet server without requiring real network connections.
 */

#ifndef TEST_MOCKS_MOCK_TELNET_SERVER_H
#define TEST_MOCKS_MOCK_TELNET_SERVER_H

#include <gmock/gmock.h>
#include "interfaces/TelnetServer.h"

class MockTelnetServer : public TelnetServer {
public:
    MOCK_METHOD(bool, start, (uint16_t), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, broadcast, (const uint8_t*, size_t), (override));
    MOCK_METHOD(void, setOnClientDataCallback, (std::function<void(int, const uint8_t*, size_t)>), (override));
    MOCK_METHOD(void, setOnClientEventCallback, (std::function<void(int, bool)>), (override));
    MOCK_METHOD(int, clientCount, (), (const, override));
};

#endif // TEST_MOCKS_MOCK_TELNET_SERVER_H
