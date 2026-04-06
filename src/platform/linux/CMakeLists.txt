# Linux platform library
add_library(platform_linux STATIC
    LinuxUartModule.cpp
    LinuxTelnetServer.cpp
    LinuxHttpServer.cpp
    LinuxDisplayModule.cpp
    LinuxWiFiStub.cpp
)

target_include_directories(platform_linux
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(platform_linux PRIVATE Threads::Threads)
