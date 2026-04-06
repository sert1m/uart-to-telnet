/**
 * @file TestConfigParser.cpp
 * @brief Property-based and unit tests for ConfigParser.
 *
 * Feature: uart-telnet-bridge
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "core/ConfigParser.h"
#include "core/DataModels.h"

// ---------------------------------------------------------------------------
// operator== for all config structs (needed for round-trip comparison)
// ---------------------------------------------------------------------------

inline bool operator==(const UartConfig& a, const UartConfig& b) {
    return a.portName == b.portName &&
           a.baudRate == b.baudRate &&
           a.dataBits == b.dataBits &&
           a.parity == b.parity &&
           a.stopBits == b.stopBits;
}

inline bool operator==(const WiFiConfig& a, const WiFiConfig& b) {
    return a.ssid == b.ssid &&
           a.password == b.password &&
           a.maxRetries == b.maxRetries;
}

inline bool operator==(const PromptRule& a, const PromptRule& b) {
    return a.trigger == b.trigger && a.response == b.response;
}

inline bool operator==(const PostCommand& a, const PostCommand& b) {
    return a.command == b.command &&
           a.expectedPrompt == b.expectedPrompt &&
           a.delayMs == b.delayMs;
}

inline bool operator==(const CommandSequence& a, const CommandSequence& b) {
    return a.rules == b.rules &&
           a.postCommands == b.postCommands &&
           a.lineEnding == b.lineEnding &&
           a.promptTimeoutMs == b.promptTimeoutMs;
}

inline bool operator==(const BridgeConfig& a, const BridgeConfig& b) {
    return a.uart == b.uart &&
           a.telnetPort == b.telnetPort &&
           a.httpPort == b.httpPort &&
           a.wifi == b.wifi &&
           a.displayTimeoutMs == b.displayTimeoutMs &&
           a.commandSequence == b.commandSequence;
}

// ---------------------------------------------------------------------------
// RapidCheck Arbitrary<> specializations for all config structs
// ---------------------------------------------------------------------------

namespace rc {

template <>
struct Arbitrary<UartConfig> {
    static Gen<UartConfig> arbitrary() {
        return gen::build<UartConfig>(
            gen::set(&UartConfig::portName, gen::nonEmpty<std::string>()),
            gen::set(&UartConfig::baudRate,
                     gen::elementOf(std::vector<uint32_t>{
                         1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800})),
            gen::set(&UartConfig::dataBits,
                     gen::map(gen::inRange(5, 9),
                              [](int v) { return static_cast<uint8_t>(v); })),
            gen::set(&UartConfig::parity,
                     gen::elementOf(std::vector<std::string>{"none", "even", "odd"})),
            gen::set(&UartConfig::stopBits,
                     gen::map(gen::inRange(1, 3),
                              [](int v) { return static_cast<uint8_t>(v); })));
    }
};

template <>
struct Arbitrary<WiFiConfig> {
    static Gen<WiFiConfig> arbitrary() {
        return gen::build<WiFiConfig>(
            gen::set(&WiFiConfig::ssid, gen::nonEmpty<std::string>()),
            gen::set(&WiFiConfig::password, gen::nonEmpty<std::string>()),
            gen::set(&WiFiConfig::maxRetries, gen::inRange(1, 100)));
    }
};

template <>
struct Arbitrary<PromptRule> {
    static Gen<PromptRule> arbitrary() {
        return gen::build<PromptRule>(
            gen::set(&PromptRule::trigger, gen::nonEmpty<std::string>()),
            gen::set(&PromptRule::response, gen::nonEmpty<std::string>()));
    }
};

template <>
struct Arbitrary<PostCommand> {
    static Gen<PostCommand> arbitrary() {
        return gen::build<PostCommand>(
            gen::set(&PostCommand::command, gen::nonEmpty<std::string>()),
            gen::set(&PostCommand::expectedPrompt, gen::arbitrary<std::string>()),
            gen::set(&PostCommand::delayMs, gen::inRange<uint32_t>(1, 30000)));
    }
};

template <>
struct Arbitrary<CommandSequence> {
    static Gen<CommandSequence> arbitrary() {
        return gen::build<CommandSequence>(
            gen::set(&CommandSequence::rules,
                     gen::container<std::vector<PromptRule>>(gen::arbitrary<PromptRule>())),
            gen::set(&CommandSequence::postCommands,
                     gen::container<std::vector<PostCommand>>(gen::arbitrary<PostCommand>())),
            gen::set(&CommandSequence::lineEnding, gen::nonEmpty<std::string>()),
            gen::set(&CommandSequence::promptTimeoutMs, gen::inRange<uint32_t>(1, 60000)));
    }
};

template <>
struct Arbitrary<BridgeConfig> {
    static Gen<BridgeConfig> arbitrary() {
        return gen::build<BridgeConfig>(
            gen::set(&BridgeConfig::uart, gen::arbitrary<UartConfig>()),
            gen::set(&BridgeConfig::telnetPort,
                     gen::map(gen::inRange(1, 65536),
                              [](int v) { return static_cast<uint16_t>(v); })),
            gen::set(&BridgeConfig::httpPort,
                     gen::map(gen::inRange(1, 65536),
                              [](int v) { return static_cast<uint16_t>(v); })),
            gen::set(&BridgeConfig::wifi, gen::arbitrary<WiFiConfig>()),
            gen::set(&BridgeConfig::displayTimeoutMs, gen::inRange<uint32_t>(1, 600000)),
            gen::set(&BridgeConfig::commandSequence, gen::arbitrary<CommandSequence>()));
    }
};

} // namespace rc

// ---------------------------------------------------------------------------
// Property 11: Configuration parsing round-trip
// Feature: uart-telnet-bridge, Property 11: Configuration parsing round-trip
// **Validates: Requirements 9.1**
// ---------------------------------------------------------------------------

RC_GTEST_PROP(ConfigParserProperties,
              configParsingRoundTrip,
              (BridgeConfig original)) {
    // Serialize to JSON
    const std::string json = ConfigParser::serializeBridgeConfig(original);

    // Parse back
    auto [parsed, error] = ConfigParser::parseJson(json);

    // Assert no error
    RC_ASSERT(error.empty());

    // Assert all fields match
    RC_ASSERT(parsed.uart == original.uart);
    RC_ASSERT(parsed.telnetPort == original.telnetPort);
    RC_ASSERT(parsed.httpPort == original.httpPort);
    RC_ASSERT(parsed.wifi == original.wifi);
    RC_ASSERT(parsed.displayTimeoutMs == original.displayTimeoutMs);
    RC_ASSERT(parsed.commandSequence == original.commandSequence);
}

// ---------------------------------------------------------------------------
// Property 12: Missing configuration field detection
// Feature: uart-telnet-bridge, Property 12: Missing configuration field detection
// **Validates: Requirements 9.2**
// ---------------------------------------------------------------------------

namespace {

/// Describes a required field: the parent object key (empty for top-level)
/// and the field name to remove.
struct RequiredField {
    std::string parent;  // "" for top-level, "uart", "wifi", "commandSequence"
    std::string field;
};

/// All required fields that must be present in a valid BridgeConfig JSON.
static const std::vector<RequiredField> kRequiredFields = {
    // Top-level
    {"", "uart"},
    {"", "telnetPort"},
    {"", "httpPort"},
    {"", "wifi"},
    {"", "displayTimeoutMs"},
    {"", "commandSequence"},
    // Under uart
    {"uart", "portName"},
    {"uart", "baudRate"},
    {"uart", "dataBits"},
    {"uart", "parity"},
    {"uart", "stopBits"},
    // Under wifi
    {"wifi", "ssid"},
    {"wifi", "password"},
    {"wifi", "maxRetries"},
    // Under commandSequence
    {"commandSequence", "rules"},
    {"commandSequence", "postCommands"},
    {"commandSequence", "lineEnding"},
    {"commandSequence", "promptTimeoutMs"},
};

} // anonymous namespace

RC_GTEST_PROP(ConfigParserProperties,
              missingFieldDetection,
              (BridgeConfig original)) {
    // 1. Serialize a valid config to JSON
    const std::string json = ConfigParser::serializeBridgeConfig(original);

    // 2. Parse into a RapidJSON Document
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    RC_ASSERT(!doc.HasParseError());
    RC_ASSERT(doc.IsObject());

    // 3. Pick one required field at random
    const auto idx = *rc::gen::inRange<size_t>(0, kRequiredFields.size());
    const auto& target = kRequiredFields[idx];

    // 4. Remove that field from the document
    if (target.parent.empty()) {
        // Top-level field
        RC_ASSERT(doc.HasMember(target.field.c_str()));
        doc.RemoveMember(target.field.c_str());
    } else {
        // Nested field under parent object
        RC_ASSERT(doc.HasMember(target.parent.c_str()));
        auto& parentObj = doc[target.parent.c_str()];
        RC_ASSERT(parentObj.IsObject());
        RC_ASSERT(parentObj.HasMember(target.field.c_str()));
        parentObj.RemoveMember(target.field.c_str());
    }

    // 5. Re-serialize the modified document to a JSON string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    const std::string modifiedJson = buffer.GetString();

    // 6. Parse the modified JSON via ConfigParser
    auto [parsed, error] = ConfigParser::parseJson(modifiedJson);

    // 7. Assert that an error was reported
    RC_ASSERT(!error.empty());

    // 8. Assert that the error mentions the removed field name
    RC_ASSERT(error.find(target.field) != std::string::npos);
}

// ---------------------------------------------------------------------------
// Unit Tests for ConfigParser
// Feature: uart-telnet-bridge, Task 2.4
// **Validates: Requirements 10.6**
// ---------------------------------------------------------------------------

// 1. Valid JSON parsed correctly with all fields populated
TEST(ConfigParserUnitTest, ValidJsonParsedCorrectly) {
    const std::string json = R"({
        "uart": {
            "portName": "/dev/ttyUSB0",
            "baudRate": 115200,
            "dataBits": 8,
            "parity": "none",
            "stopBits": 1
        },
        "telnetPort": 2323,
        "httpPort": 8080,
        "wifi": {
            "ssid": "TestNetwork",
            "password": "secret123",
            "maxRetries": 5
        },
        "displayTimeoutMs": 30000,
        "commandSequence": {
            "rules": [
                { "trigger": "login:", "response": "admin" },
                { "trigger": "password:", "response": "pass123" }
            ],
            "postCommands": [
                { "command": "journalctl -f", "expectedPrompt": "~$", "delayMs": 1000 }
            ],
            "lineEnding": "\r\n",
            "promptTimeoutMs": 5000
        }
    })";

    auto [config, error] = ConfigParser::parseJson(json);

    EXPECT_TRUE(error.empty()) << "Unexpected error: " << error;

    // UART fields
    EXPECT_EQ(config.uart.portName, "/dev/ttyUSB0");
    EXPECT_EQ(config.uart.baudRate, 115200u);
    EXPECT_EQ(config.uart.dataBits, 8);
    EXPECT_EQ(config.uart.parity, "none");
    EXPECT_EQ(config.uart.stopBits, 1);

    // Network ports
    EXPECT_EQ(config.telnetPort, 2323);
    EXPECT_EQ(config.httpPort, 8080);

    // WiFi
    EXPECT_EQ(config.wifi.ssid, "TestNetwork");
    EXPECT_EQ(config.wifi.password, "secret123");
    EXPECT_EQ(config.wifi.maxRetries, 5);

    // Display
    EXPECT_EQ(config.displayTimeoutMs, 30000u);

    // CommandSequence
    ASSERT_EQ(config.commandSequence.rules.size(), 2u);
    EXPECT_EQ(config.commandSequence.rules[0].trigger, "login:");
    EXPECT_EQ(config.commandSequence.rules[0].response, "admin");
    EXPECT_EQ(config.commandSequence.rules[1].trigger, "password:");
    EXPECT_EQ(config.commandSequence.rules[1].response, "pass123");

    ASSERT_EQ(config.commandSequence.postCommands.size(), 1u);
    EXPECT_EQ(config.commandSequence.postCommands[0].command, "journalctl -f");
    EXPECT_EQ(config.commandSequence.postCommands[0].expectedPrompt, "~$");
    EXPECT_EQ(config.commandSequence.postCommands[0].delayMs, 1000u);

    EXPECT_EQ(config.commandSequence.lineEnding, "\r\n");
    EXPECT_EQ(config.commandSequence.promptTimeoutMs, 5000u);
}

// 2. Missing required field produces descriptive error
TEST(ConfigParserUnitTest, MissingRequiredFieldProducesError) {
    // Valid JSON but missing uart.portName
    const std::string json = R"({
        "uart": {
            "baudRate": 115200,
            "dataBits": 8,
            "parity": "none",
            "stopBits": 1
        },
        "telnetPort": 23,
        "httpPort": 80,
        "wifi": { "ssid": "net", "password": "pw", "maxRetries": 3 },
        "displayTimeoutMs": 60000,
        "commandSequence": {
            "rules": [],
            "postCommands": [],
            "lineEnding": "\n",
            "promptTimeoutMs": 5000
        }
    })";

    auto [config, error] = ConfigParser::parseJson(json);

    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("portName"), std::string::npos)
        << "Error should mention the missing field 'portName', got: " << error;
}

// 3. Malformed JSON returns parse error
TEST(ConfigParserUnitTest, MalformedJsonReturnsParseError) {
    const std::string json = "{bad json}";

    auto [config, error] = ConfigParser::parseJson(json);

    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("parse error"), std::string::npos)
        << "Error should mention 'parse error', got: " << error;
}

// 4. CommandSequence parsing from HTTP body JSON
TEST(ConfigParserUnitTest, CommandSequenceParsing) {
    const std::string json = R"({
        "commandSequence": {
            "rules": [
                { "trigger": "login:", "response": "root" }
            ],
            "postCommands": [
                { "command": "ls -la", "expectedPrompt": "#", "delayMs": 200 }
            ],
            "lineEnding": "\n",
            "promptTimeoutMs": 3000
        }
    })";

    auto [seq, error] = ConfigParser::parseCommandSequence(json);

    EXPECT_TRUE(error.empty()) << "Unexpected error: " << error;

    ASSERT_EQ(seq.rules.size(), 1u);
    EXPECT_EQ(seq.rules[0].trigger, "login:");
    EXPECT_EQ(seq.rules[0].response, "root");

    ASSERT_EQ(seq.postCommands.size(), 1u);
    EXPECT_EQ(seq.postCommands[0].command, "ls -la");
    EXPECT_EQ(seq.postCommands[0].expectedPrompt, "#");
    EXPECT_EQ(seq.postCommands[0].delayMs, 200u);

    EXPECT_EQ(seq.lineEnding, "\n");
    EXPECT_EQ(seq.promptTimeoutMs, 3000u);
}

// 5. CommandSequence serialization round-trip
TEST(ConfigParserUnitTest, CommandSequenceSerialization) {
    CommandSequence original;
    original.rules = {{"login:", "admin"}, {"password:", "secret"}};
    PostCommand pc;
    pc.command = "uptime";
    pc.expectedPrompt = "$";
    pc.delayMs = 750;
    original.postCommands = {pc};
    original.lineEnding = "\r\n";
    original.promptTimeoutMs = 4000;

    // Serialize
    const std::string json = ConfigParser::serializeCommandSequence(original);

    // Parse back
    auto [parsed, error] = ConfigParser::parseCommandSequence(json);

    EXPECT_TRUE(error.empty()) << "Unexpected error: " << error;
    EXPECT_EQ(parsed.rules.size(), original.rules.size());
    EXPECT_EQ(parsed.postCommands.size(), original.postCommands.size());
    EXPECT_EQ(parsed.lineEnding, original.lineEnding);
    EXPECT_EQ(parsed.promptTimeoutMs, original.promptTimeoutMs);

    // Verify content
    ASSERT_EQ(parsed.rules.size(), 2u);
    EXPECT_EQ(parsed.rules[0].trigger, "login:");
    EXPECT_EQ(parsed.rules[0].response, "admin");
    EXPECT_EQ(parsed.rules[1].trigger, "password:");
    EXPECT_EQ(parsed.rules[1].response, "secret");

    ASSERT_EQ(parsed.postCommands.size(), 1u);
    EXPECT_EQ(parsed.postCommands[0].command, "uptime");
    EXPECT_EQ(parsed.postCommands[0].expectedPrompt, "$");
    EXPECT_EQ(parsed.postCommands[0].delayMs, 750u);
}

// 6. parseFile with non-existent path returns error mentioning the file path
TEST(ConfigParserUnitTest, ParseFileNotFound) {
    const std::string fakePath = "/tmp/nonexistent_config_12345.json";

    auto [config, error] = ConfigParser::parseFile(fakePath);

    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find(fakePath), std::string::npos)
        << "Error should mention the file path, got: " << error;
}
