/**
 * @file PlatformFactory.cpp
 * @brief Implementation of PlatformFactory.
 */

#include "PlatformFactory.h"

#include <stdexcept>

// Linux platform headers
#include "platform/linux/LinuxDisplayModule.h"
#include "platform/linux/LinuxHttpServer.h"
#include "platform/linux/LinuxStorageModule.h"
#include "platform/linux/LinuxTelnetServer.h"
#include "platform/linux/LinuxUartModule.h"
#include "platform/linux/LinuxWiFiStub.h"

// ESP32 platform headers — only available when building with the ESP32 SDK.
#ifdef PLATFORM_ESP32
#include "platform/esp32/Esp32HttpServer.h"
#include "platform/esp32/Esp32TelnetServer.h"
#include "platform/esp32/Esp32UartModule.h"
#include "platform/esp32/Esp32WiFiModule.h"
#include "platform/esp32/M5StackDisplayModule.h"
#endif

PlatformModules PlatformFactory::create(const std::string& platformId,
                                        const std::string& logDir,
                                        const std::string& networkInterface) {
    if (platformId == "linux") {
        PlatformModules modules;
        modules.uart    = std::make_unique<LinuxUartModule>();
        modules.telnet  = std::make_unique<LinuxTelnetServer>();
        modules.http    = std::make_unique<LinuxHttpServer>();
        modules.wifi    = std::make_unique<LinuxWiFiStub>(networkInterface);

        // Storage: only created when logDir is configured
        LinuxStorageModule* rawStorage = nullptr;
        if (!logDir.empty()) {
            auto storage = std::make_unique<LinuxStorageModule>(logDir);
            rawStorage = storage.get();
            modules.storage = std::move(storage);
        }

        // Display receives a pointer to storage (may be nullptr)
        auto display = std::make_unique<LinuxDisplayModule>();
        display->setStorage(rawStorage);
        modules.display = std::move(display);

        return modules;
    }

#ifdef PLATFORM_ESP32
    if (platformId == "esp32") {
        PlatformModules modules;
        modules.uart    = std::make_unique<Esp32UartModule>();
        modules.telnet  = std::make_unique<Esp32TelnetServer>();
        modules.http    = std::make_unique<Esp32HttpServer>();
        modules.display = std::make_unique<M5StackDisplayModule>();
        modules.wifi    = std::make_unique<Esp32WiFiModule>();
        return modules;
    }
#endif

    throw std::runtime_error("Unknown platform: " + platformId);
}
