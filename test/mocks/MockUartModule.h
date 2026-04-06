/**
 * @file MockUartModule.h
 * @brief Google Mock implementation of UartModule for unit testing.
 *
 * Allows tests to set expectations and verify interactions with the
 * UART module without requiring real hardware.
 */

#ifndef TEST_MOCKS_MOCK_UART_MODULE_H
#define TEST_MOCKS_MOCK_UART_MODULE_H

#include <gmock/gmock.h>
#include "interfaces/UartModule.h"

class MockUartModule : public UartModule {
public:
    MOCK_METHOD(bool, open, (const UartConfig&), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(int, write, (const uint8_t*, size_t), (override));
    MOCK_METHOD(void, setOnDataCallback, (std::function<void(const uint8_t*, size_t)>), (override));
    MOCK_METHOD(bool, isOpen, (), (const, override));
    MOCK_METHOD(bool, reopen, (), (override));
};

#endif // TEST_MOCKS_MOCK_UART_MODULE_H
