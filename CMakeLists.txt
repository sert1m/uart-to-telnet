cmake_minimum_required(VERSION 3.14)
project(uartTelnetBridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for IDE navigation (clangd, Kiro, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform selection cache variable (linux or esp32)
set(PLATFORM "linux" CACHE STRING "Target platform: linux, esp32")

# Core library (platform-agnostic sources)
add_library(core
    src/core/BridgeController.cpp
    src/core/AutoResponder.cpp
    src/core/ConfigParser.cpp
)
target_include_directories(core PUBLIC include)

# Use system-installed RapidJSON (install with: sudo apt install rapidjson-dev)
find_package(RapidJSON REQUIRED)
target_include_directories(core PUBLIC ${RAPIDJSON_INCLUDE_DIRS})

# Platform-specific sources (selected by PLATFORM variable)
add_subdirectory(src/platform/${PLATFORM})

# Main executable
add_executable(uartTelnetBridge src/main.cpp src/PlatformFactory.cpp)
target_link_libraries(uartTelnetBridge core platform_${PLATFORM})

# Tests (Linux host only, opt-in via BUILD_TESTS)
if(PLATFORM STREQUAL "linux" AND BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
