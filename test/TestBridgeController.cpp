/**
 * @file TestBridgeController.cpp
 * @brief Property-based and unit tests for BridgeController.
 *
 * Feature: uart-telnet-bridge
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "core/BridgeController.h"
#include "core/AutoResponder.h"
#include "core/DataModels.h"

#include "mocks/MockUartModule.h"
#include "mocks/MockTelnetServer.h"
#include "mocks/MockHttpServer.h"
#include "mocks/MockDisplayModule.h"
#include "mocks/MockWiFiModule.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::SaveArg;
using ::testing::InSequence;
using ::testing::NiceMock;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

/// Build a valid BridgeConfig suitable for startup tests.
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

/// Configure all mocks so that startup() succeeds by default.
void expectSuccessfulStartup(NiceMock<MockWiFiModule>& wifi,
                             NiceMock<MockUartModule>& uart,
                             NiceMock<MockTelnetServer>& telnet,
                             NiceMock<MockHttpServer>& http,
                             NiceMock<MockDisplayModule>& display) {
    ON_CALL(wifi, connect(_)).WillByDefault(Return(true));
    ON_CALL(uart, open(_)).WillByDefault(Return(true));
    ON_CALL(telnet, start(_)).WillByDefault(Return(true));
    ON_CALL(http, start(_)).WillByDefault(Return(true));
}

} // anonymous namespace

// ===========================================================================
// Property Tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Property 1: UART-to-Telnet data integrity
// Feature: uart-telnet-bridge, Property 1: UART-to-Telnet data integrity
// **Validates: Requirements 2.2, 2.3**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(BridgeControllerProperties,
              uartToTelnetDataIntegrity,
              ()) {
    // Generate random byte data (non-empty)
    const auto data = *rc::gen::nonEmpty<std::vector<uint8_t>>();

    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    // No-op write callback for AutoResponder
    AutoResponder autoResponder([](const uint8_t*, size_t) {});

    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    // Capture the UART data callback
    std::function<void(const uint8_t*, size_t)> uartDataCallback;
    ON_CALL(uart, setOnDataCallback(_))
        .WillByDefault(SaveArg<0>(&uartDataCallback));

    expectSuccessfulStartup(wifi, uart, telnet, http, display);

    auto cfg = makeValidConfig();
    bool started = controller.startup(cfg);
    RC_ASSERT(started);
    RC_ASSERT(uartDataCallback != nullptr);

    // Capture what broadcast receives
    std::vector<uint8_t> broadcastedData;
    ON_CALL(telnet, broadcast(_, _))
        .WillByDefault(Invoke([&broadcastedData](const uint8_t* d, size_t len) {
            broadcastedData.insert(broadcastedData.end(), d, d + len);
        }));

    // Invoke the captured UART callback with random data
    uartDataCallback(data.data(), data.size());

    // Verify exact bytes were broadcast
    RC_ASSERT(broadcastedData == data);
}

// ---------------------------------------------------------------------------
// Property 2: Telnet-to-UART data integrity
// Feature: uart-telnet-bridge, Property 2: Telnet-to-UART data integrity
// **Validates: Requirements 3.1, 3.3**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(BridgeControllerProperties,
              telnetToUartDataIntegrity,
              ()) {
    const auto data = *rc::gen::nonEmpty<std::vector<uint8_t>>();
    const auto clientId = *rc::gen::inRange(1, 100);

    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    AutoResponder autoResponder([](const uint8_t*, size_t) {});

    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    // Capture the Telnet client data callback
    std::function<void(int, const uint8_t*, size_t)> telnetDataCallback;
    ON_CALL(telnet, setOnClientDataCallback(_))
        .WillByDefault(SaveArg<0>(&telnetDataCallback));

    expectSuccessfulStartup(wifi, uart, telnet, http, display);

    auto cfg = makeValidConfig();
    bool started = controller.startup(cfg);
    RC_ASSERT(started);
    RC_ASSERT(telnetDataCallback != nullptr);

    // Capture what uart.write receives
    std::vector<uint8_t> writtenData;
    ON_CALL(uart, write(_, _))
        .WillByDefault(Invoke([&writtenData](const uint8_t* d, size_t len) -> int {
            writtenData.insert(writtenData.end(), d, d + len);
            return static_cast<int>(len);
        }));

    // Invoke the captured Telnet callback with random data
    telnetDataCallback(clientId, data.data(), data.size());

    // Verify exact bytes were written to UART
    RC_ASSERT(writtenData == data);
}

// ---------------------------------------------------------------------------
// Property 3: No command echo to other Telnet clients
// Feature: uart-telnet-bridge, Property 3: No command echo to other Telnet clients
// **Validates: Requirements 3.2**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(BridgeControllerProperties,
              noCommandEchoToOtherClients,
              ()) {
    const auto data = *rc::gen::nonEmpty<std::vector<uint8_t>>();
    const auto clientId = *rc::gen::inRange(1, 100);

    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    AutoResponder autoResponder([](const uint8_t*, size_t) {});

    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    // Capture the Telnet client data callback
    std::function<void(int, const uint8_t*, size_t)> telnetDataCallback;
    ON_CALL(telnet, setOnClientDataCallback(_))
        .WillByDefault(SaveArg<0>(&telnetDataCallback));

    expectSuccessfulStartup(wifi, uart, telnet, http, display);

    // Track whether broadcast was called
    bool broadcastCalled = false;
    ON_CALL(telnet, broadcast(_, _))
        .WillByDefault(Invoke([&broadcastCalled](const uint8_t*, size_t) {
            broadcastCalled = true;
        }));

    auto cfg = makeValidConfig();
    bool started = controller.startup(cfg);
    RC_ASSERT(started);
    RC_ASSERT(telnetDataCallback != nullptr);

    // Reset after startup (startup may wire callbacks that don't trigger broadcast)
    broadcastCalled = false;

    // Invoke the Telnet client data callback — this should NOT trigger broadcast
    telnetDataCallback(clientId, data.data(), data.size());

    RC_ASSERT(!broadcastCalled);
}

// ---------------------------------------------------------------------------
// Property 13: UART throughput computation
// Feature: uart-telnet-bridge, Property 13: UART throughput computation
// **Validates: Requirements 7.3**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(BridgeControllerProperties,
              uartThroughputComputation,
              ()) {
    // Generate a number of UART data chunks (1..20)
    const auto numChunks = *rc::gen::inRange(1, 21);

    NiceMock<MockUartModule> uart;
    NiceMock<MockTelnetServer> telnet;
    NiceMock<MockHttpServer> http;
    NiceMock<MockDisplayModule> display;
    NiceMock<MockWiFiModule> wifi;

    AutoResponder autoResponder([](const uint8_t*, size_t) {});

    BridgeController controller(uart, telnet, http, display, wifi, autoResponder);

    // Capture the UART data callback
    std::function<void(const uint8_t*, size_t)> uartDataCallback;
    ON_CALL(uart, setOnDataCallback(_))
        .WillByDefault(SaveArg<0>(&uartDataCallback));

    expectSuccessfulStartup(wifi, uart, telnet, http, display);

    auto cfg = makeValidConfig();
    bool started = controller.startup(cfg);
    RC_ASSERT(started);
    RC_ASSERT(uartDataCallback != nullptr);

    // Feed known byte counts via the UART callback
    uint32_t totalRxBytes = 0;
    for (int i = 0; i < numChunks; ++i) {
        // Generate a chunk of 1..200 bytes
        auto chunkSize = *rc::gen::inRange(1, 201);
        std::vector<uint8_t> chunk(static_cast<size_t>(chunkSize), 0xAA);
        uartDataCallback(chunk.data(), chunk.size());
        totalRxBytes += static_cast<uint32_t>(chunkSize);
    }

    // Tick exactly 1000ms to trigger throughput calculation
    controller.tick(1000);

    // Expected RX bytes per sec = totalRxBytes / 1.0 = totalRxBytes
    uint32_t actual = controller.getRxBytesPerSec();
    int32_t diff = static_cast<int32_t>(actual) - static_cast<int32_t>(totalRxBytes);
    RC_ASSERT(diff >= -1 && diff <= 1);
}

// ===========================================================================
// Unit Tests — BridgeControllerUnitTest fixture
// ===========================================================================

class BridgeControllerUnitTest : public ::testing::Test {
protected:
    NiceMock<MockUartModule> uart_;
    NiceMock<MockTelnetServer> telnet_;
    NiceMock<MockHttpServer> http_;
    NiceMock<MockDisplayModule> display_;
    NiceMock<MockWiFiModule> wifi_;

    // Real AutoResponder with a no-op write callback
    AutoResponder autoResponder_{[](const uint8_t*, size_t) {}};

    std::unique_ptr<BridgeController> controller_;

    // Captured callbacks
    std::function<void(const uint8_t*, size_t)> uartDataCallback_;
    std::function<void(int, const uint8_t*, size_t)> telnetDataCallback_;

    BridgeConfig config_;

    void SetUp() override {
        config_ = makeValidConfig();

        // Capture callbacks when they are set
        ON_CALL(uart_, setOnDataCallback(_))
            .WillByDefault(SaveArg<0>(&uartDataCallback_));
        ON_CALL(telnet_, setOnClientDataCallback(_))
            .WillByDefault(SaveArg<0>(&telnetDataCallback_));

        controller_ = std::make_unique<BridgeController>(
            uart_, telnet_, http_, display_, wifi_, autoResponder_);
    }

    /// Helper: configure all mocks for successful startup
    void setupSuccessfulStartup() {
        expectSuccessfulStartup(wifi_, uart_, telnet_, http_, display_);
    }
};

// ---------------------------------------------------------------------------
// 1. UartOpenFailureTerminates
//    Mock uart.open returns false, verify startup returns false
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, UartOpenFailureTerminates) {
    ON_CALL(wifi_, connect(_)).WillByDefault(Return(true));
    ON_CALL(uart_, open(_)).WillByDefault(Return(false));

    EXPECT_FALSE(controller_->startup(config_));
}

// ---------------------------------------------------------------------------
// 2. TelnetPortInUseTerminates
//    Mock telnet.start returns false, verify startup returns false
//    and uart.close was called (cleanup)
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, TelnetPortInUseTerminates) {
    ON_CALL(wifi_, connect(_)).WillByDefault(Return(true));
    ON_CALL(uart_, open(_)).WillByDefault(Return(true));
    ON_CALL(telnet_, start(_)).WillByDefault(Return(false));

    EXPECT_CALL(uart_, close()).Times(1);

    EXPECT_FALSE(controller_->startup(config_));
}

// ---------------------------------------------------------------------------
// 3. HttpPortInUseTerminates
//    Mock http.start returns false, verify startup returns false
//    and telnet.stop + uart.close called
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, HttpPortInUseTerminates) {
    ON_CALL(wifi_, connect(_)).WillByDefault(Return(true));
    ON_CALL(uart_, open(_)).WillByDefault(Return(true));
    ON_CALL(telnet_, start(_)).WillByDefault(Return(true));
    ON_CALL(http_, start(_)).WillByDefault(Return(false));

    EXPECT_CALL(telnet_, stop()).Times(1);
    EXPECT_CALL(uart_, close()).Times(1);

    EXPECT_FALSE(controller_->startup(config_));
}

// ---------------------------------------------------------------------------
// 4. UartConnectionLossTriggersReopen
//    Call onUartConnectionLost, verify uart.reopen was called
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, UartConnectionLossTriggersReopen) {
    EXPECT_CALL(uart_, reopen()).Times(1).WillOnce(Return(true));

    controller_->onUartConnectionLost();
}

// ---------------------------------------------------------------------------
// 5. GracefulShutdownReversesOrder
//    Call startup then shutdown, verify stop/close calls happen in
//    reverse order using InSequence
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, GracefulShutdownReversesOrder) {
    setupSuccessfulStartup();
    ASSERT_TRUE(controller_->startup(config_));

    {
        InSequence seq;
        EXPECT_CALL(http_, stop());
        EXPECT_CALL(telnet_, stop());
        EXPECT_CALL(uart_, close());
        EXPECT_CALL(wifi_, disconnect());
    }

    controller_->shutdown();
}

// ---------------------------------------------------------------------------
// 6. TelnetBroadcastContinuesDuringAutoResponder
//    Verify that UART data callback calls both autoResponder.process
//    and telnet.broadcast
// ---------------------------------------------------------------------------
TEST_F(BridgeControllerUnitTest, TelnetBroadcastContinuesDuringAutoResponder) {
    // Configure AutoResponder with a rule so it will process data
    CommandSequence seq;
    PromptRule rule;
    rule.trigger = "login:";
    rule.response = "admin";
    seq.rules = {rule};
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 5000;
    autoResponder_.setConfig(seq);

    setupSuccessfulStartup();

    // Track broadcast calls
    std::vector<uint8_t> broadcastedData;
    ON_CALL(telnet_, broadcast(_, _))
        .WillByDefault(Invoke([&broadcastedData](const uint8_t* d, size_t len) {
            broadcastedData.insert(broadcastedData.end(), d, d + len);
        }));

    ASSERT_TRUE(controller_->startup(config_));
    ASSERT_TRUE(uartDataCallback_ != nullptr);

    // Send UART data that contains the trigger
    std::string uartData = "Welcome\nlogin:";
    uartDataCallback_(reinterpret_cast<const uint8_t*>(uartData.data()),
                      uartData.size());

    // Verify broadcast was called with the same data (even though AutoResponder
    // also processed it)
    std::string broadcastStr(broadcastedData.begin(), broadcastedData.end());
    EXPECT_EQ(broadcastStr, uartData);
}
