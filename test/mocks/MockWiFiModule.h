/**
 * @file MockWiFiModule.h
 * @brief Google Mock implementation of WiFiModule for unit testing.
 *
 * Allows tests to set expectations and verify interactions with the
 * WiFi module without requiring real wireless hardware.
 */

#ifndef TEST_MOCKS_MOCK_WIFI_MODULE_H
#define TEST_MOCKS_MOCK_WIFI_MODULE_H

#include <gmock/gmock.h>
#include "interfaces/WiFiModule.h"

class MockWiFiModule : public WiFiModule {
public:
    MOCK_METHOD(bool, connect, (const WiFiConfig&), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(std::string, getIpAddress, (), (const, override));
    MOCK_METHOD(int, getSignalStrengthDbm, (), (const, override));
};

#endif // TEST_MOCKS_MOCK_WIFI_MODULE_H
