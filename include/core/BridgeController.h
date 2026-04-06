/**
 * @file BridgeController.h
 * @brief Central orchestrator for the UART-Telnet Bridge application.
 *
 * Owns references to all platform interfaces, wires up data callbacks,
 * registers HTTP handlers, tracks UART throughput, and manages the
 * application lifecycle (startup, tick, shutdown).
 *
 * @see Requirement 1.1-1.5, 2.1-2.5, 3.1-3.3, 4.4, 5.1-5.5, 5.7, 7.3, 8.3
 */

#ifndef CORE_BRIDGE_CONTROLLER_H
#define CORE_BRIDGE_CONTROLLER_H

#include "core/AutoResponder.h"
#include "core/DataModels.h"

class UartModule;
class TelnetServer;
class HttpServer;
class DisplayModule;
class WiFiModule;

/**
 * @brief Central orchestrator that wires platform modules and core logic.
 *
 * Accepts all platform interfaces and the AutoResponder via constructor
 * (dependency injection). Manages startup sequencing, data forwarding
 * callbacks, HTTP handler registration, throughput tracking, and
 * graceful shutdown.
 */
class BridgeController {
public:
    /**
     * @brief Construct a BridgeController with all dependencies.
     * @param uart      Reference to the UART communication module.
     * @param telnet    Reference to the Telnet server module.
     * @param http      Reference to the HTTP configuration server.
     * @param display   Reference to the status display module.
     * @param wifi      Reference to the WiFi connectivity module.
     * @param autoResponder Reference to the automatic prompt responder.
     */
    BridgeController(UartModule& uart,
                     TelnetServer& telnet,
                     HttpServer& http,
                     DisplayModule& display,
                     WiFiModule& wifi,
                     AutoResponder& autoResponder);

    /**
     * @brief Initialize all modules, wire callbacks, and register HTTP handlers.
     *
     * Initialization order: display, WiFi, UART, Telnet, HTTP.
     * On any fatal failure the method logs a descriptive error, closes
     * already-opened modules in reverse order, and returns false.
     *
     * @param config Full bridge configuration.
     * @return true if all modules initialized successfully, false on fatal error.
     *
     * @see Requirement 1.1, 1.4, 2.1, 2.5, 5.1, 5.7
     */
    bool startup(const BridgeConfig& config);

    /**
     * @brief Shut down all modules in reverse initialization order.
     *
     * Shutdown order: HTTP, Telnet, UART, WiFi, display (reverse of startup).
     */
    void shutdown();

    /**
     * @brief Periodic tick — advance AutoResponder, update throughput, refresh display.
     *
     * Should be called from the main loop at a regular interval.
     *
     * @param elapsedMs Milliseconds elapsed since the last tick call.
     *
     * @see Requirement 7.3
     */
    void tick(uint32_t elapsedMs);

    /**
     * @brief Handle UART connection loss by attempting to reopen the port.
     *
     * @see Requirement 1.5
     */
    void onUartConnectionLost();

    /**
     * @brief Get the most recently computed RX bytes-per-second.
     * @return UART receive throughput in bytes/sec.
     */
    uint32_t getRxBytesPerSec() const;

    /**
     * @brief Get the most recently computed TX bytes-per-second.
     * @return UART transmit throughput in bytes/sec.
     */
    uint32_t getTxBytesPerSec() const;

private:
    /// @brief Wire UART data callback (forward to AutoResponder + Telnet broadcast).
    void wireUartCallback();

    /// @brief Wire Telnet client data callback (forward to UART write).
    void wireTelnetCallback();

    /// @brief Register HTTP route handlers on the HTTP server.
    void registerHttpHandlers();

    /// @brief Update throughput counters and refresh the display.
    void updateThroughputAndDisplay();

    UartModule& uart_;
    TelnetServer& telnet_;
    HttpServer& http_;
    DisplayModule& display_;
    WiFiModule& wifi_;
    AutoResponder& autoResponder_;

    BridgeConfig config_;               ///< Stored configuration from startup.

    // Throughput tracking
    uint32_t rxByteCount_ = 0;          ///< RX bytes since last throughput calculation.
    uint32_t txByteCount_ = 0;          ///< TX bytes since last throughput calculation.
    uint32_t throughputElapsedMs_ = 0;  ///< Time accumulated since last throughput update.
    uint32_t rxBytesPerSec_ = 0;        ///< Last computed RX throughput.
    uint32_t txBytesPerSec_ = 0;        ///< Last computed TX throughput.

    static constexpr uint32_t kThroughputIntervalMs = 1000; ///< Throughput update interval.
};

#endif // CORE_BRIDGE_CONTROLLER_H
