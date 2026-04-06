/**
 * @file PlatformFactory.h
 * @brief Factory for creating platform-specific module implementations.
 *
 * Maps a platform identifier string to concrete implementations of all
 * platform interfaces. Supports adding new platforms by registering their
 * implementations without modifying existing code.
 */

#ifndef PLATFORM_FACTORY_H
#define PLATFORM_FACTORY_H

#include "interfaces/UartModule.h"
#include "interfaces/TelnetServer.h"
#include "interfaces/HttpServer.h"
#include "interfaces/DisplayModule.h"
#include "interfaces/WiFiModule.h"

#include <memory>
#include <string>

/**
 * @brief Holds unique_ptr instances for all platform module implementations.
 *
 * Returned by PlatformFactory::create() to provide the caller with ownership
 * of all platform-specific modules needed by the bridge.
 */
struct PlatformModules {
    std::unique_ptr<UartModule> uart;         ///< UART communication module.
    std::unique_ptr<TelnetServer> telnet;     ///< Telnet server module.
    std::unique_ptr<HttpServer> http;         ///< HTTP server module.
    std::unique_ptr<DisplayModule> display;   ///< Status display module.
    std::unique_ptr<WiFiModule> wifi;         ///< WiFi connectivity module.
};

/**
 * @brief Factory that creates platform-specific module implementations.
 *
 * Given a platform identifier (e.g., "linux", "esp32"), instantiates the
 * correct concrete implementations of each platform interface.
 */
class PlatformFactory {
public:
    /**
     * @brief Create all platform modules for the given platform identifier.
     * @param platformId Platform name, e.g., "linux" or "esp32".
     * @return PlatformModules struct with all modules instantiated.
     * @throws std::runtime_error if @p platformId is not recognized.
     */
    static PlatformModules create(const std::string& platformId);
};

#endif // PLATFORM_FACTORY_H
