/**
 * @file ConfigParser.h
 * @brief JSON configuration parser and serializer for the UART-Telnet Bridge.
 *
 * Provides static methods to parse JSON configuration files and strings into
 * BridgeConfig and CommandSequence structs, and to serialize them back to JSON.
 * Uses the RapidJSON library for all JSON operations.
 *
 * @see Requirement 5.2, 5.3, 5.6, 9.1, 9.2, 9.3
 */

#ifndef CORE_CONFIG_PARSER_H
#define CORE_CONFIG_PARSER_H

#include "DataModels.h"
#include <string>
#include <utility>

/**
 * @brief Static utility class for JSON configuration parsing and serialization.
 *
 * All methods are static — no instance state is needed. Parsing methods return
 * a pair of (result, errorString). An empty error string indicates success;
 * a non-empty string describes the specific validation or parse failure.
 */
class ConfigParser {
public:
    /**
     * @brief Parse a JSON configuration file into a BridgeConfig.
     *
     * Reads the file at @p filePath, parses its JSON content, and validates
     * that all required fields are present.
     *
     * @param filePath Path to the JSON configuration file.
     * @return A pair of (BridgeConfig, errorString). The error string is empty
     *         on success, or contains a description of the failure (file I/O
     *         error, parse error, or missing field name).
     *
     * @see Requirement 9.1, 9.2, 9.3
     */
    static std::pair<BridgeConfig, std::string> parseFile(const std::string& filePath);

    /**
     * @brief Parse a JSON string into a BridgeConfig.
     *
     * Validates that all required fields are present in the JSON document.
     * Returns an error identifying the specific missing field on failure.
     *
     * @param json JSON string representing a full BridgeConfig.
     * @return A pair of (BridgeConfig, errorString). Empty error on success.
     *
     * @see Requirement 9.1, 9.2
     */
    static std::pair<BridgeConfig, std::string> parseJson(const std::string& json);

    /**
     * @brief Serialize a BridgeConfig to a JSON string.
     *
     * Produces a JSON document containing all BridgeConfig fields, suitable
     * for round-trip testing or writing back to a configuration file.
     *
     * @param config The BridgeConfig to serialize.
     * @return A JSON string representation of @p config.
     */
    static std::string serializeBridgeConfig(const BridgeConfig& config);

    /**
     * @brief Parse an HTTP request body JSON string into a CommandSequence.
     *
     * Expects a JSON object with a top-level "commandSequence" key (matching
     * the HTTP POST /config body format). Validates required fields within
     * the command sequence.
     *
     * @param json JSON string from an HTTP request body.
     * @return A pair of (CommandSequence, errorString). Empty error on success.
     *
     * @see Requirement 5.2, 5.6
     */
    static std::pair<CommandSequence, std::string> parseCommandSequence(const std::string& json);

    /**
     * @brief Serialize a CommandSequence to a JSON string.
     *
     * Produces a JSON document with a top-level "commandSequence" key,
     * matching the HTTP GET /config response format.
     *
     * @param seq The CommandSequence to serialize.
     * @return A JSON string representation of @p seq.
     *
     * @see Requirement 5.3, 5.6
     */
    static std::string serializeCommandSequence(const CommandSequence& seq);

private:
    ConfigParser() = default;
};

#endif // CORE_CONFIG_PARSER_H
