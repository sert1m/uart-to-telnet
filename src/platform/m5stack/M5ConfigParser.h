/**
 * @file M5ConfigParser.h
 * @brief Arduino-compatible ConfigParser using ArduinoJson instead of RapidJSON.
 *
 * Provides the same static API as ConfigParser but uses ArduinoJson,
 * which is available in the PlatformIO Arduino environment.
 * Only implements parseCommandSequence and serializeCommandSequence
 * (needed for HTTP runtime config). parseFile/parseJson are not needed
 * since M5Stack uses hardcoded config.
 */

#ifndef CORE_CONFIG_PARSER_H
#define CORE_CONFIG_PARSER_H

#include "core/DataModels.h"
#include <string>
#include <utility>

class ConfigParser {
public:
    static std::pair<CommandSequence, std::string> parseCommandSequence(const std::string& json);
    static std::string serializeCommandSequence(const CommandSequence& seq);

    // Not needed on M5Stack (hardcoded config), but declared to satisfy linker
    static std::pair<BridgeConfig, std::string> parseFile(const std::string&) {
        return {{}, "Not supported on M5Stack"};
    }
    static std::pair<BridgeConfig, std::string> parseJson(const std::string&) {
        return {{}, "Not supported on M5Stack"};
    }
    static std::string serializeBridgeConfig(const BridgeConfig&) {
        return "{}";
    }
};

#endif // CORE_CONFIG_PARSER_H
