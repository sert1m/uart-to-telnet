include(FetchContent)

# Fetch Google Test (includes gmock)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch RapidCheck for property-based testing
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG        master
)
set(RC_ENABLE_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidcheck)

# Test executable
add_executable(uartTelnetBridgeTests
    TestMain.cpp
    TestConfigParser.cpp
    TestAutoResponder.cpp
    TestBridgeController.cpp
    TestHttpHandlers.cpp
    TestPlatformFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/PlatformFactory.cpp
)

target_link_libraries(uartTelnetBridgeTests
    PRIVATE
        core
        platform_linux
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        rapidcheck
        rapidcheck_gtest
)

target_include_directories(uartTelnetBridgeTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/test
)

include(GoogleTest)
gtest_discover_tests(uartTelnetBridgeTests)
