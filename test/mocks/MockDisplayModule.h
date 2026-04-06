/**
 * @file MockDisplayModule.h
 * @brief Google Mock implementation of DisplayModule for unit testing.
 *
 * Allows tests to set expectations and verify interactions with the
 * display module without requiring real display hardware.
 */

#ifndef TEST_MOCKS_MOCK_DISPLAY_MODULE_H
#define TEST_MOCKS_MOCK_DISPLAY_MODULE_H

#include <gmock/gmock.h>
#include "interfaces/DisplayModule.h"

class MockDisplayModule : public DisplayModule {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (const DisplayStatus&), (override));
    MOCK_METHOD(void, setBacklight, (bool), (override));
    MOCK_METHOD(void, onButtonPress, (), (override));
};

#endif // TEST_MOCKS_MOCK_DISPLAY_MODULE_H
