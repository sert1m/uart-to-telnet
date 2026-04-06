cmake_minimum_required(VERSION 3.14)
project(uartTelnetBridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform selection cache variable (linux or esp32)
set(PLATFORM "linux" CACHE STRING "Target platform: linux, esp32")

# Core library (platform-agnostic sources)
add_library(core
    src/core/BridgeController.cpp
    src/core/AutoResponder.cpp
    src/core/ConfigParser.cpp
)
target_include_directories(core PUBLIC include)

# Fetch RapidJSON for JSON parsing (header-only, skip its tests/examples)
include(FetchContent)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        v1.1.0
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Populate(rapidjson)
target_include_directories(core PUBLIC ${rapidjson_SOURCE_DIR}/include)

# Platform-specific sources (selected by PLATFORM variable)
add_subdirectory(src/platform/${PLATFORM})

# Main executable
add_executable(uartTelnetBridge src/main.cpp src/PlatformFactory.cpp)
target_link_libraries(uartTelnetBridge core platform_${PLATFORM})

# Tests (Linux host only)
if(PLATFORM STREQUAL "linux")
    enable_testing()
    add_subdirectory(test)
endif()
