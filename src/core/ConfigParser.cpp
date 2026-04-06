/**
 * @file ConfigParser.cpp
 * @brief Implementation of ConfigParser JSON parsing and serialization.
 */

#include "core/ConfigParser.h"

#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/**
 * Check that a JSON object contains a member with the expected type.
 * Returns an error string if the member is missing; empty string on success.
 */
std::string requireString(const Value& obj, const char* field, const char* context) {
    if (!obj.HasMember(field) || !obj[field].IsString()) {
        return std::string("Missing or invalid field: ") + context + "." + field;
    }
    return {};
}

std::string requireUint(const Value& obj, const char* field, const char* context) {
    if (!obj.HasMember(field) || !obj[field].IsUint()) {
        return std::string("Missing or invalid field: ") + context + "." + field;
    }
    return {};
}

std::string requireInt(const Value& obj, const char* field, const char* context) {
    if (!obj.HasMember(field) || !obj[field].IsInt()) {
        return std::string("Missing or invalid field: ") + context + "." + field;
    }
    return {};
}

std::string requireObject(const Value& obj, const char* field, const char* context) {
    if (!obj.HasMember(field) || !obj[field].IsObject()) {
        return std::string("Missing or invalid field: ") + context + "." + field;
    }
    return {};
}

std::string requireArray(const Value& obj, const char* field, const char* context) {
    if (!obj.HasMember(field) || !obj[field].IsArray()) {
        return std::string("Missing or invalid field: ") + context + "." + field;
    }
    return {};
}

// Parse a CommandSequence from a RapidJSON Value (shared by both parseJson and parseCommandSequence).
std::string parseCommandSequenceValue(const Value& csObj, CommandSequence& seq) {
    std::string err;

    // rules
    err = requireArray(csObj, "rules", "commandSequence");
    if (!err.empty()) return err;

    for (SizeType i = 0; i < csObj["rules"].Size(); ++i) {
        const auto& ruleVal = csObj["rules"][i];
        std::string ctx = "commandSequence.rules[" + std::to_string(i) + "]";
        if (!ruleVal.IsObject()) return "Invalid element at " + ctx;

        err = requireString(ruleVal, "trigger", ctx.c_str());
        if (!err.empty()) return err;
        err = requireString(ruleVal, "response", ctx.c_str());
        if (!err.empty()) return err;

        PromptRule rule;
        rule.trigger = ruleVal["trigger"].GetString();
        rule.response = ruleVal["response"].GetString();
        seq.rules.push_back(std::move(rule));
    }

    // postCommands
    err = requireArray(csObj, "postCommands", "commandSequence");
    if (!err.empty()) return err;

    for (SizeType i = 0; i < csObj["postCommands"].Size(); ++i) {
        const auto& pcVal = csObj["postCommands"][i];
        std::string ctx = "commandSequence.postCommands[" + std::to_string(i) + "]";
        if (!pcVal.IsObject()) return "Invalid element at " + ctx;

        err = requireString(pcVal, "command", ctx.c_str());
        if (!err.empty()) return err;

        PostCommand pc;
        pc.command = pcVal["command"].GetString();
        if (pcVal.HasMember("expectedPrompt") && pcVal["expectedPrompt"].IsString()) {
            pc.expectedPrompt = pcVal["expectedPrompt"].GetString();
        }
        if (pcVal.HasMember("delayMs") && pcVal["delayMs"].IsUint()) {
            pc.delayMs = pcVal["delayMs"].GetUint();
        }
        seq.postCommands.push_back(std::move(pc));
    }

    // lineEnding
    err = requireString(csObj, "lineEnding", "commandSequence");
    if (!err.empty()) return err;
    seq.lineEnding = csObj["lineEnding"].GetString();

    // promptTimeoutMs
    err = requireUint(csObj, "promptTimeoutMs", "commandSequence");
    if (!err.empty()) return err;
    seq.promptTimeoutMs = csObj["promptTimeoutMs"].GetUint();

    return {};
}

// Serialize a CommandSequence into a RapidJSON Value.
void serializeCommandSequenceValue(const CommandSequence& seq, Value& csObj, Document::AllocatorType& alloc) {
    csObj.SetObject();

    // rules
    Value rulesArr(kArrayType);
    for (const auto& rule : seq.rules) {
        Value ruleObj(kObjectType);
        ruleObj.AddMember("trigger", Value(rule.trigger.c_str(), alloc), alloc);
        ruleObj.AddMember("response", Value(rule.response.c_str(), alloc), alloc);
        rulesArr.PushBack(ruleObj, alloc);
    }
    csObj.AddMember("rules", rulesArr, alloc);

    // postCommands
    Value pcArr(kArrayType);
    for (const auto& pc : seq.postCommands) {
        Value pcObj(kObjectType);
        pcObj.AddMember("command", Value(pc.command.c_str(), alloc), alloc);
        pcObj.AddMember("expectedPrompt", Value(pc.expectedPrompt.c_str(), alloc), alloc);
        pcObj.AddMember("delayMs", pc.delayMs, alloc);
        pcArr.PushBack(pcObj, alloc);
    }
    csObj.AddMember("postCommands", pcArr, alloc);

    csObj.AddMember("lineEnding", Value(seq.lineEnding.c_str(), alloc), alloc);
    csObj.AddMember("promptTimeoutMs", seq.promptTimeoutMs, alloc);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::pair<BridgeConfig, std::string> ConfigParser::parseFile(const std::string& filePath) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return {{}, "Cannot open configuration file: " + filePath};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return parseJson(oss.str());
}

std::pair<BridgeConfig, std::string> ConfigParser::parseJson(const std::string& json) {
    Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        return {{}, std::string("JSON parse error: ") + GetParseError_En(doc.GetParseError())
                     + " at offset " + std::to_string(doc.GetErrorOffset())};
    }
    if (!doc.IsObject()) {
        return {{}, "JSON root must be an object"};
    }

    BridgeConfig config;
    std::string err;

    // uart
    err = requireObject(doc, "uart", "config");
    if (!err.empty()) return {{}, err};
    {
        const auto& u = doc["uart"];
        err = requireString(u, "portName", "uart");
        if (!err.empty()) return {{}, err};
        err = requireUint(u, "baudRate", "uart");
        if (!err.empty()) return {{}, err};
        err = requireUint(u, "dataBits", "uart");
        if (!err.empty()) return {{}, err};
        err = requireString(u, "parity", "uart");
        if (!err.empty()) return {{}, err};
        err = requireUint(u, "stopBits", "uart");
        if (!err.empty()) return {{}, err};

        config.uart.portName = u["portName"].GetString();
        config.uart.baudRate = u["baudRate"].GetUint();
        config.uart.dataBits = static_cast<uint8_t>(u["dataBits"].GetUint());
        config.uart.parity = u["parity"].GetString();
        config.uart.stopBits = static_cast<uint8_t>(u["stopBits"].GetUint());
    }

    // telnetPort
    err = requireUint(doc, "telnetPort", "config");
    if (!err.empty()) return {{}, err};
    config.telnetPort = static_cast<uint16_t>(doc["telnetPort"].GetUint());

    // httpPort
    err = requireUint(doc, "httpPort", "config");
    if (!err.empty()) return {{}, err};
    config.httpPort = static_cast<uint16_t>(doc["httpPort"].GetUint());

    // wifi
    err = requireObject(doc, "wifi", "config");
    if (!err.empty()) return {{}, err};
    {
        const auto& w = doc["wifi"];
        err = requireString(w, "ssid", "wifi");
        if (!err.empty()) return {{}, err};
        err = requireString(w, "password", "wifi");
        if (!err.empty()) return {{}, err};
        err = requireInt(w, "maxRetries", "wifi");
        if (!err.empty()) return {{}, err};

        config.wifi.ssid = w["ssid"].GetString();
        config.wifi.password = w["password"].GetString();
        config.wifi.maxRetries = w["maxRetries"].GetInt();
    }

    // displayTimeoutMs
    err = requireUint(doc, "displayTimeoutMs", "config");
    if (!err.empty()) return {{}, err};
    config.displayTimeoutMs = doc["displayTimeoutMs"].GetUint();

    // commandSequence
    err = requireObject(doc, "commandSequence", "config");
    if (!err.empty()) return {{}, err};
    err = parseCommandSequenceValue(doc["commandSequence"], config.commandSequence);
    if (!err.empty()) return {{}, err};

    return {config, {}};
}

std::string ConfigParser::serializeBridgeConfig(const BridgeConfig& config) {
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // uart
    Value uartObj(kObjectType);
    uartObj.AddMember("portName", Value(config.uart.portName.c_str(), alloc), alloc);
    uartObj.AddMember("baudRate", config.uart.baudRate, alloc);
    uartObj.AddMember("dataBits", config.uart.dataBits, alloc);
    uartObj.AddMember("parity", Value(config.uart.parity.c_str(), alloc), alloc);
    uartObj.AddMember("stopBits", config.uart.stopBits, alloc);
    doc.AddMember("uart", uartObj, alloc);

    doc.AddMember("telnetPort", config.telnetPort, alloc);
    doc.AddMember("httpPort", config.httpPort, alloc);

    // wifi
    Value wifiObj(kObjectType);
    wifiObj.AddMember("ssid", Value(config.wifi.ssid.c_str(), alloc), alloc);
    wifiObj.AddMember("password", Value(config.wifi.password.c_str(), alloc), alloc);
    wifiObj.AddMember("maxRetries", config.wifi.maxRetries, alloc);
    doc.AddMember("wifi", wifiObj, alloc);

    doc.AddMember("displayTimeoutMs", config.displayTimeoutMs, alloc);

    // commandSequence
    Value csObj;
    serializeCommandSequenceValue(config.commandSequence, csObj, alloc);
    doc.AddMember("commandSequence", csObj, alloc);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

std::pair<CommandSequence, std::string> ConfigParser::parseCommandSequence(const std::string& json) {
    Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        return {{}, std::string("JSON parse error: ") + GetParseError_En(doc.GetParseError())
                     + " at offset " + std::to_string(doc.GetErrorOffset())};
    }
    if (!doc.IsObject()) {
        return {{}, "JSON root must be an object"};
    }

    std::string err = requireObject(doc, "commandSequence", "body");
    if (!err.empty()) return {{}, err};

    CommandSequence seq;
    err = parseCommandSequenceValue(doc["commandSequence"], seq);
    if (!err.empty()) return {{}, err};

    return {seq, {}};
}

std::string ConfigParser::serializeCommandSequence(const CommandSequence& seq) {
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    Value csObj;
    serializeCommandSequenceValue(seq, csObj, alloc);
    doc.AddMember("commandSequence", csObj, alloc);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}
