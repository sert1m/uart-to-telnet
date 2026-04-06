/**
 * @file TestHttpHandlers.cpp
 * @brief Property-based and unit tests for HTTP handler logic.
 *
 * Feature: uart-telnet-bridge
 *
 * Tests the HTTP route handlers registered by BridgeController::registerHttpHandlers():
 *   POST /config — update AutoResponder configuration
 *   GET  /config — return current AutoResponder configuration
 *   *    *       — 404 for unknown endpoints
 *
 * The approach: create a BridgeController with NiceMock objects, capture the
 * handler functions registered via MockHttpServer::registerHandler, then invoke
 * those handlers directly with test data.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "core/BridgeController.h"
#include "core/AutoResponder.h"
#include "core/ConfigParser.h"
#include "core/DataModels.h"

#include "mocks/MockUartModule.h"
#include "mocks/MockTelnetServer.h"
#include "mocks/MockHttpServer.h"
#include "mocks/MockDisplayModule.h"
#include "mocks/MockWiFiModule.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::NiceMock;

// ---------------------------------------------------------------------------
// operator== for comparison in round-trip tests
// ---------------------------------------------------------------------------

static bool operator==(const PromptRule& a, const PromptRule& b) {
    return a.trigger == b.trigger && a.response == b.response;
}

static bool operator==(const PostCommand& a, const PostCommand& b) {
    return a.command == b.command &&
           a.expectedPrompt == b.expectedPrompt &&
           a.delayMs == b.delayMs;
}

static bool operator==(const CommandSequence& a, const CommandSequence& b) {
    return a.rules == b.rules &&
           a.postCommands == b.postCommands &&
           a.lineEnding == b.lineEnding &&
           a.promptTimeoutMs == b.promptTimeoutMs;
}

// ---------------------------------------------------------------------------
// RapidCheck Arbitrary<> specializations (consistent with TestConfigParser.cpp)
// ---------------------------------------------------------------------------

namespace rc {

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

} // namespace rc

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Key type for the captured handler map: (method, path)
using HandlerKey = std::pair<std::string, std::string>;
using HandlerFunc = std::function<HttpResponse(const std::string&)>;
using HandlerMap = std::map<HandlerKey, HandlerFunc>;

/// Build a minimal valid BridgeConfig for startup.
BridgeConfig makeValidConfig() {
    BridgeConfig cfg;
    cfg.uart.portName = "/dev/ttyUSB0";
    cfg.uart.baudRate = 115200;
    cfg.uart.dataBits = 8;
    cfg.uart.parity = "none";
    cfg.uart.stopBits = 1;
    cfg.telnetPort = 2323;
    cfg.httpPort = 8080;
    cfg.wifi.ssid = "TestSSID";
    cfg.wifi.password = "TestPass";
    cfg.wifi.maxRetries = 3;
    cfg.displayTimeoutMs = 60000;
    return cfg;
}

/// Configure all mocks so that startup() succeeds.
void expectSuccessfulStartup(NiceMock<MockWiFiModule>& wifi,
                             NiceMock<MockUartModule>& uart,
                             NiceMock<MockTelnetServer>& telnet,
                             NiceMock<MockHttpServer>& http) {
    ON_CALL(wifi, connect(_)).WillByDefault(Return(true));
    ON_CALL(uart, open(_)).WillByDefault(Return(true));
    ON_CALL(telnet, start(_)).WillByDefault(Return(true));
    ON_CALL(http, start(_)).WillByDefault(Return(true));
}

/// Set up the MockHttpServer to capture registered handlers into the map.
void captureHandlers(NiceMock<MockHttpServer>& http, HandlerMap& handlers) {
    ON_CALL(http, registerHandler(_, _, _))
        .WillByDefault(Invoke(
            [&handlers](const std::string& method,
                        const std::string& path,
                        std::function<HttpResponse(const std::string&)> handler) {
                handlers[{method, path}] = std::move(handler);
            }));
}

} // anonymous namespace

// ===========================================================================
// Test Fixture
// ===========================================================================

class HttpHandlerUnitTest : public ::testing::Test {
protected:
    NiceMock<MockUartModule> uart_;
    NiceMock<MockTelnetServer> telnet_;
    NiceMock<MockHttpServer> http_;
    NiceMock<MockDisplayModule> display_;
    NiceMock<MockWiFiModule> wifi_;

    AutoResponder autoResponder_{[](const uint8_t*, size_t) {}};

    std::unique_ptr<BridgeController> controller_;
    HandlerMap handlers_;

    void SetUp() override {
        captureHandlers(http_, handlers_);
        expectSuccessfulStartup(wifi_, uart_, telnet_, http_);

        controller_ = std::make_unique<BridgeController>(
            uart_, telnet_, http_, display_, wifi_, autoResponder_);

        auto cfg = makeValidConfig();
        ASSERT_TRUE(controller_->startup(cfg));

        // Verify we captured the expected handlers
        ASSERT_NE(handlers_.find({"POST", "/config"}), handlers_.end());
        ASSERT_NE(handlers_.find({"GET", "/config"}), handlers_.end());
        ASSERT_NE(handlers_.find({"*", "*"}), handlers_.end());
    }
};

// ===========================================================================
// Property Tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Property 9: HTTP configuration round-trip
// Feature: uart-telnet-bridge, Property 9: HTTP configuration round-trip
// **Validates: Requirements 5.2, 5.3**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(HttpHandlerProperties,
              httpConfigurationRoundTrip,
              (CommandSequence original)) {
    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    HandlerMap handlers;
    captureHandlers(http, handlers);
    expectSuccessfulStartup(wifi, uart, telnet, http);

    AutoResponder autoResponder([](const uint8_t*, size_t) {});
    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    auto cfg = makeValidConfig();
    RC_ASSERT(controller.startup(cfg));

    RC_ASSERT(handlers.count({"POST", "/config"}) > 0);
    RC_ASSERT(handlers.count({"GET", "/config"}) > 0);

    auto& postHandler = handlers[{"POST", "/config"}];
    auto& getHandler = handlers[{"GET", "/config"}];

    // Serialize the random CommandSequence to JSON
    std::string jsonBody = ConfigParser::serializeCommandSequence(original);

    // POST it
    HttpResponse postResp = postHandler(jsonBody);
    RC_ASSERT(postResp.statusCode == 200);

    // GET it back
    HttpResponse getResp = getHandler("");
    RC_ASSERT(getResp.statusCode == 200);

    // Parse the GET response and compare
    auto [parsed, error] = ConfigParser::parseCommandSequence(getResp.body);
    RC_ASSERT(error.empty());
    RC_ASSERT(parsed == original);
}

// ---------------------------------------------------------------------------
// Property 10: HTTP error responses for invalid requests
// Feature: uart-telnet-bridge, Property 10: HTTP error responses for invalid requests
// **Validates: Requirements 5.4, 5.5**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(HttpHandlerProperties,
              httpErrorResponsesForInvalidRequests,
              ()) {
    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    HandlerMap handlers;
    captureHandlers(http, handlers);
    expectSuccessfulStartup(wifi, uart, telnet, http);

    AutoResponder autoResponder([](const uint8_t*, size_t) {});
    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    auto cfg = makeValidConfig();
    RC_ASSERT(controller.startup(cfg));

    RC_ASSERT(handlers.count({"POST", "/config"}) > 0);
    RC_ASSERT(handlers.count({"*", "*"}) > 0);

    auto& postHandler = handlers[{"POST", "/config"}];
    auto& wildcardHandler = handlers[{"*", "*"}];

    // Generate a random non-JSON string (ensure it's not valid JSON)
    auto malformed = *rc::gen::nonEmpty<std::string>();
    // Prepend something that guarantees invalid JSON
    malformed = "{{invalid json " + malformed;

    // POST malformed JSON should return 400
    HttpResponse postResp = postHandler(malformed);
    RC_ASSERT(postResp.statusCode == 400);
    RC_ASSERT(postResp.body.find("error") != std::string::npos);

    // Wildcard handler should return 404
    HttpResponse notFoundResp = wildcardHandler("");
    RC_ASSERT(notFoundResp.statusCode == 404);
    RC_ASSERT(notFoundResp.body.find("error") != std::string::npos);
}

// ===========================================================================
// Unit Tests
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. ValidJsonUpdatesConfig
//    POST valid JSON, verify 200 and AutoResponder config updated
//    **Validates: Requirements 10.5**
// ---------------------------------------------------------------------------
TEST_F(HttpHandlerUnitTest, ValidJsonUpdatesConfig) {
    const std::string json = R"({
        "commandSequence": {
            "rules": [
                { "trigger": "login:", "response": "admin" },
                { "trigger": "password:", "response": "secret123" }
            ],
            "postCommands": [
                { "command": "journalctl -f", "expectedPrompt": "~$", "delayMs": 1000 }
            ],
            "lineEnding": "\n",
            "promptTimeoutMs": 5000
        }
    })";

    auto& postHandler = handlers_[{"POST", "/config"}];
    HttpResponse resp = postHandler(json);

    EXPECT_EQ(resp.statusCode, 200);

    // Verify AutoResponder was updated
    const auto& config = autoResponder_.getConfig();
    ASSERT_EQ(config.rules.size(), 2u);
    EXPECT_EQ(config.rules[0].trigger, "login:");
    EXPECT_EQ(config.rules[0].response, "admin");
    EXPECT_EQ(config.rules[1].trigger, "password:");
    EXPECT_EQ(config.rules[1].response, "secret123");
    ASSERT_EQ(config.postCommands.size(), 1u);
    EXPECT_EQ(config.postCommands[0].command, "journalctl -f");
    EXPECT_EQ(config.lineEnding, "\n");
    EXPECT_EQ(config.promptTimeoutMs, 5000u);
}

// ---------------------------------------------------------------------------
// 2. MalformedJsonReturns400
//    POST malformed JSON, verify 400 with error message
//    **Validates: Requirements 10.5**
// ---------------------------------------------------------------------------
TEST_F(HttpHandlerUnitTest, MalformedJsonReturns400) {
    auto& postHandler = handlers_[{"POST", "/config"}];
    HttpResponse resp = postHandler("{bad json}");

    EXPECT_EQ(resp.statusCode, 400);
    EXPECT_NE(resp.body.find("error"), std::string::npos);
}

// ---------------------------------------------------------------------------
// 3. UnknownEndpointReturns404
//    Call wildcard handler, verify 404
//    **Validates: Requirements 10.5**
// ---------------------------------------------------------------------------
TEST_F(HttpHandlerUnitTest, UnknownEndpointReturns404) {
    auto& wildcardHandler = handlers_[{"*", "*"}];
    HttpResponse resp = wildcardHandler("");

    EXPECT_EQ(resp.statusCode, 404);
    EXPECT_NE(resp.body.find("Not found"), std::string::npos);
}

// ---------------------------------------------------------------------------
// 4. GetReturnsCurrentConfig
//    Set config via POST, then GET, verify response contains the config
//    **Validates: Requirements 10.5**
// ---------------------------------------------------------------------------
TEST_F(HttpHandlerUnitTest, GetReturnsCurrentConfig) {
    // POST a known config
    const std::string json = R"({
        "commandSequence": {
            "rules": [
                { "trigger": "user:", "response": "root" }
            ],
            "postCommands": [],
            "lineEnding": "\r\n",
            "promptTimeoutMs": 3000
        }
    })";

    auto& postHandler = handlers_[{"POST", "/config"}];
    HttpResponse postResp = postHandler(json);
    ASSERT_EQ(postResp.statusCode, 200);

    // GET the config back
    auto& getHandler = handlers_[{"GET", "/config"}];
    HttpResponse getResp = getHandler("");

    EXPECT_EQ(getResp.statusCode, 200);
    EXPECT_EQ(getResp.contentType, "application/json");

    // Parse the response and verify
    auto [seq, error] = ConfigParser::parseCommandSequence(getResp.body);
    EXPECT_TRUE(error.empty()) << "Parse error: " << error;

    ASSERT_EQ(seq.rules.size(), 1u);
    EXPECT_EQ(seq.rules[0].trigger, "user:");
    EXPECT_EQ(seq.rules[0].response, "root");
    EXPECT_TRUE(seq.postCommands.empty());
    EXPECT_EQ(seq.lineEnding, "\r\n");
    EXPECT_EQ(seq.promptTimeoutMs, 3000u);
}
