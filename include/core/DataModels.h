/**
 * @file DataModels.h
 * @brief Data model structs for the UART-Telnet Bridge application.
 *
 * Defines configuration, status, and HTTP response structures used
 * throughout the bridge core logic and platform interfaces.
 */

#ifndef CORE_DATA_MODELS_H
#define CORE_DATA_MODELS_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief UART port configuration.
 *
 * Holds all parameters needed to open and configure a hardware UART port.
 * Used by UartModule::open() during bridge startup.
 *
 * @see Requirement 1.1
 */
struct UartConfig {
    std::string portName;       ///< Device path, e.g., "/dev/ttyUSB0" or "UART1".
    uint32_t baudRate;          ///< Baud rate, e.g., 115200.
    uint8_t dataBits;           ///< Data bits per frame: 5, 6, 7, or 8.
    std::string parity;         ///< Parity mode: "none", "even", or "odd".
    uint8_t stopBits;           ///< Stop bits: 1 or 2.
};

/**
 * @brief WiFi access point configuration.
 *
 * Contains credentials and retry settings for WiFi connectivity.
 * Used by WiFiModule::connect() on platforms with wireless support.
 */
struct WiFiConfig {
    std::string ssid;           ///< WiFi network SSID.
    std::string password;       ///< WiFi network password.
    int maxRetries;             ///< Max connection retries before restarting the cycle.
};

/**
 * @brief A single prompt-response rule for the AutoResponder.
 *
 * Maps a trigger string found in UART output to a response string
 * that the AutoResponder sends back to the UART device.
 *
 * @see Requirement 4.1
 */
struct PromptRule {
    std::string trigger;        ///< String to match in UART output.
    std::string response;       ///< String to send when the trigger is matched.
};

/**
 * @brief A command to execute after all PromptRules in a CommandSequence complete.
 *
 * PostCommands are sent to the UART device in order after all prompt-response
 * rules have been processed. Each command can optionally wait for an expected
 * prompt before the next command is sent.
 *
 * @see Requirement 4.7, 4.10
 */
struct PostCommand {
    std::string command;                    ///< Command string to send to the UART device.
    std::string expectedPrompt;            ///< Optional prompt to wait for before sending the next command.
    uint32_t delayMs = 500;                ///< Delay in ms before sending the next command (default: 500).
};

/**
 * @brief An ordered sequence of prompt-response rules followed by post-login commands.
 *
 * Defines an automated interaction sequence (e.g., login, password entry,
 * then a set of commands to execute). Rules are processed in order; post-commands
 * execute after all rules complete.
 *
 * @see Requirement 4.1, 4.6, 4.7
 */
struct CommandSequence {
    std::vector<PromptRule> rules;          ///< Ordered list of prompt-response pairs.
    std::vector<PostCommand> postCommands;  ///< Commands to execute after all rules complete.
    std::string lineEnding = "\n";          ///< Line ending appended to responses (default: "\n").
    uint32_t promptTimeoutMs = 0;          ///< Timeout in ms for unmatched prompts.
};

/**
 * @brief Top-level application configuration.
 *
 * Aggregates all configuration needed to initialize the bridge: UART settings,
 * network ports, WiFi credentials, display timeout, log directory, and the
 * auto-responder command sequence.
 *
 * @see Requirement 9.1
 */
struct BridgeConfig {
    UartConfig uart;                        ///< UART port settings.
    uint16_t telnetPort = 23;              ///< Telnet server listening port (default: 23).
    uint16_t httpPort = 80;                ///< HTTP server listening port (default: 80).
    WiFiConfig wifi;                        ///< WiFi access point settings.
    uint32_t displayTimeoutMs = 60000;     ///< Display backlight timeout in ms (default: 60000).
    std::string logDir;                     ///< Directory for log files (empty = no file logging).
    std::string networkInterface;           ///< Network interface name for IP display, e.g. "eth0" (empty = auto-detect).
    CommandSequence commandSequence;        ///< AutoResponder command sequence configuration.
};

/**
 * @brief Current bridge status for display rendering.
 *
 * Captures a snapshot of the bridge's operational state, including WiFi
 * connectivity and UART throughput metrics. Used by DisplayModule::update().
 *
 * @see Requirement 7.1
 */
struct DisplayStatus {
    bool wifiConnected;                     ///< true if WiFi is connected.
    int signalStrengthDbm;                  ///< WiFi signal strength in dBm (0 on Linux).
    std::string ipAddress;                  ///< Current IP address string.
    uint32_t uartRxBytesPerSec;            ///< UART receive throughput in bytes/sec.
    uint32_t uartTxBytesPerSec;            ///< UART transmit throughput in bytes/sec.
};

/**
 * @brief HTTP response returned by registered handlers.
 *
 * Encapsulates the status code, body, and content type for responses
 * generated by HTTP route handlers.
 *
 * @see Requirement 5.6
 */
struct HttpResponse {
    int statusCode;                         ///< HTTP status code (e.g., 200, 400, 404).
    std::string body;                       ///< Response body content.
    std::string contentType = "application/json"; ///< Content-Type header value (default: "application/json").
};

#endif // CORE_DATA_MODELS_H
