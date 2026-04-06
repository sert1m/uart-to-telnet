# ESP32 platform library
# Only compiled when PLATFORM=esp32 (not included in Linux builds)
add_library(platform_esp32 STATIC
    Esp32UartModule.cpp
    Esp32TelnetServer.cpp
    Esp32HttpServer.cpp
    M5StackDisplayModule.cpp
    Esp32WiFiModule.cpp
)

target_include_directories(platform_esp32 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
