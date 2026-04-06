/**
 * @file main.cpp
 * @brief Entry point for the UART-Telnet Bridge application.
 *
 * Parses command-line arguments, loads configuration, creates platform
 * modules, and runs the main bridge loop with graceful signal handling.
 *
 * Usage: ./uartTelnetBridge [config_file]
 *   config_file  Path to JSON configuration file (default: "config.json")
 *
 * @see Requirement 9.1, 9.2, 9.3
 */

#include "PlatformFactory.h"
#include "core/AutoResponder.h"
#include "core/BridgeController.h"
#include "core/ConfigParser.h"
#include "core/DataModels.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

/// @brief Global flag set to false by signal handlers to request shutdown.
static std::atomic<bool> running{true};

/**
 * @brief Signal handler for SIGINT and SIGTERM.
 * @param signum The signal number received.
 */
static void signalHandler(int /*signum*/) {
    running.store(false);
}

int main(int argc, char* argv[]) {
    // Parse command-line argument for config file path
    std::string configPath = "config.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    // Load configuration
    auto [config, error] = ConfigParser::parseFile(configPath);
    if (!error.empty()) {
        std::cerr << "Error loading config \"" << configPath << "\": " << error << std::endl;
        return 1;
    }

    // Create platform modules (hardcode "linux" for now)
    PlatformModules modules;
    try {
        modules = PlatformFactory::create("linux");
    } catch (const std::exception& ex) {
        std::cerr << "Error creating platform modules: " << ex.what() << std::endl;
        return 1;
    }

    // Construct AutoResponder with a write callback that writes to UART
    AutoResponder autoResponder([&modules](const uint8_t* data, size_t length) {
        modules.uart->write(data, length);
    });

    // Construct BridgeController with all platform modules + AutoResponder
    BridgeController controller(
        *modules.uart,
        *modules.telnet,
        *modules.http,
        *modules.display,
        *modules.wifi,
        autoResponder
    );

    // Start up the bridge
    if (!controller.startup(config)) {
        std::cerr << "Bridge startup failed" << std::endl;
        return 1;
    }

    // Install signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "UART-Telnet Bridge running. Press Ctrl+C to stop." << std::endl;

    // Main loop: tick every 10ms
    while (running.load()) {
        controller.tick(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Shutting down..." << std::endl;
    controller.shutdown();

    return 0;
}
