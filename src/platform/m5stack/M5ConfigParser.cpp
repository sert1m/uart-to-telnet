/**
 * @file M5ConfigParser.cpp
 * @brief Arduino-compatible ConfigParser using ArduinoJson v7.
 */

#include "M5ConfigParser.h"
#include <ArduinoJson.h>

std::pair<CommandSequence, std::string> ConfigParser::parseCommandSequence(const std::string& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return {{}, std::string("JSON parse error: ") + err.c_str()};
    }

    if (!doc["commandSequence"].is<JsonObject>()) {
        return {{}, "Missing or invalid field: body.commandSequence"};
    }

    JsonObject cs = doc["commandSequence"];
    CommandSequence seq;

    if (!cs["rules"].is<JsonArray>()) {
        return {{}, "Missing or invalid field: commandSequence.rules"};
    }
    for (JsonObject rule : cs["rules"].as<JsonArray>()) {
        if (!rule["trigger"].is<const char*>() || !rule["response"].is<const char*>()) {
            return {{}, "Missing trigger or response in rule"};
        }
        PromptRule pr;
        pr.trigger = rule["trigger"].as<std::string>();
        pr.response = rule["response"].as<std::string>();
        seq.rules.push_back(pr);
    }

    if (!cs["postCommands"].is<JsonArray>()) {
        return {{}, "Missing or invalid field: commandSequence.postCommands"};
    }
    for (JsonObject pc : cs["postCommands"].as<JsonArray>()) {
        if (!pc["command"].is<const char*>()) {
            return {{}, "Missing command in postCommand"};
        }
        PostCommand cmd;
        cmd.command = pc["command"].as<std::string>();
        cmd.expectedPrompt = pc["expectedPrompt"].is<const char*>() ? pc["expectedPrompt"].as<std::string>() : "";
        cmd.delayMs = pc["delayMs"].is<uint32_t>() ? pc["delayMs"].as<uint32_t>() : 500;
        seq.postCommands.push_back(cmd);
    }

    if (!cs["lineEnding"].is<const char*>()) {
        return {{}, "Missing or invalid field: commandSequence.lineEnding"};
    }
    seq.lineEnding = cs["lineEnding"].as<std::string>();

    if (!cs["promptTimeoutMs"].is<uint32_t>()) {
        return {{}, "Missing or invalid field: commandSequence.promptTimeoutMs"};
    }
    seq.promptTimeoutMs = cs["promptTimeoutMs"].as<uint32_t>();

    return {seq, {}};
}

std::string ConfigParser::serializeCommandSequence(const CommandSequence& seq) {
    JsonDocument doc;
    JsonObject cs = doc["commandSequence"].to<JsonObject>();

    JsonArray rules = cs["rules"].to<JsonArray>();
    for (const auto& rule : seq.rules) {
        JsonObject r = rules.add<JsonObject>();
        r["trigger"] = rule.trigger;
        r["response"] = rule.response;
    }

    JsonArray postCmds = cs["postCommands"].to<JsonArray>();
    for (const auto& pc : seq.postCommands) {
        JsonObject p = postCmds.add<JsonObject>();
        p["command"] = pc.command;
        p["expectedPrompt"] = pc.expectedPrompt;
        p["delayMs"] = pc.delayMs;
    }

    cs["lineEnding"] = seq.lineEnding;
    cs["promptTimeoutMs"] = seq.promptTimeoutMs;

    std::string output;
    serializeJson(doc, output);
    return output;
}
