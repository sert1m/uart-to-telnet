/**
 * @file PlatformFactory.cpp
 * @brief Implementation of PlatformFactory.
 */

#include "PlatformFactory.h"

#include <stdexcept>

// Linux platform headers
#include "platform/linux/LinuxUartModule.h"
#include "platform/linux/LinuxTelnetServer.h"
#include "platform/linux/LinuxHttpServer.h"
#include "platform/linux/LinuxDisplayModule.h"
#include "platform/linux/LinuxWiFiStub.h"

// ESP32 platform headers — only available when building with the ESP32 SDK.
// When PLATFORM=esp32, the ESP-IDF toolchain provides the required headers.
// On Linux builds, the esp32 subdirectory is not compiled, so these headers
// are not included.
#ifdef PLATFORM_ESP32
#include "platform/esp32/Esp32UartModule.h"
#include "platform/esp32/Esp32TelnetServer.h"
#include "platform/esp32/Esp32HttpServer.h"
#include "platform/esp32/M5StackDisplayModule.h"
#include "platform/esp32/Esp32WiFiModule.h"
#endif

PlatformModules PlatformFactory::create(const std::string& platformId) {
    if (platformId == "linux") {
        PlatformModules modules;
        modules.uart = std::make_unique<LinuxUartModule>();
        modules.telnet = std::make_unique<LinuxTelnetServer>();
        modules.http = std::make_unique<LinuxHttpServer>();
        modules.display = std::make_unique<LinuxDisplayModule>();
        modules.wifi = std::make_unique<LinuxWiFiStub>();
        return modules;
    }

#ifdef PLATFORM_ESP32
    if (platformId == "esp32") {
        PlatformModules modules;
        modules.uart = std::make_unique<Esp32UartModule>();
        modules.telnet = std::make_unique<Esp32TelnetServer>();
        modules.http = std::make_unique<Esp32HttpServer>();
        modules.display = std::make_unique<M5StackDisplayModule>();
        modules.wifi = std::make_unique<Esp32WiFiModule>();
        return modules;
    }
#endif

    throw std::runtime_error("Unknown platform: " + platformId);
}
