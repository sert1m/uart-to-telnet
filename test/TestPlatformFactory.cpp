/**
 * @file TestPlatformFactory.cpp
 * @brief Unit tests for PlatformFactory.
 */

#include "PlatformFactory.h"

#include <gtest/gtest.h>
#include <stdexcept>

/**
 * @brief Verify that create("linux") returns non-null pointers for all modules.
 */
TEST(PlatformFactoryTest, LinuxPlatformReturnsValidModules) {
    PlatformModules modules = PlatformFactory::create("linux");

    EXPECT_NE(modules.uart, nullptr);
    EXPECT_NE(modules.telnet, nullptr);
    EXPECT_NE(modules.http, nullptr);
    EXPECT_NE(modules.display, nullptr);
    EXPECT_NE(modules.wifi, nullptr);
}

/**
 * @brief Verify that create("unknown") throws std::runtime_error.
 */
TEST(PlatformFactoryTest, UnknownPlatformThrows) {
    EXPECT_THROW(PlatformFactory::create("unknown"), std::runtime_error);
}
