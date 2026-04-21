/**
 * @file main.cpp
 * @brief M5Stack Arduino entry point for the UART-Telnet Bridge.
 *
 * Hardcoded configuration — edit the values below for your setup.
 */

#include <M5Stack.h>
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

#include "core/AutoResponder.h"
#include "core/BridgeController.h"
#include "core/DataModels.h"

#include "platform/m5stack/M5UartModule.h"
#include "platform/m5stack/M5TelnetServer.h"
#include "platform/m5stack/M5HttpServer.h"
#include "platform/m5stack/M5DisplayModule.h"
#include "platform/m5stack/M5WiFiModule.h"
#include "platform/m5stack/M5StorageModule.h"

// ============================================================
// Hardcoded configuration — edit these for your environment
// ============================================================

static BridgeConfig makeConfig() {
    BridgeConfig cfg;

    // UART: GPIO 16 (RX) / GPIO 17 (TX) on M5Stack Basic
    cfg.uart.portName = "UART2";
    cfg.uart.baudRate = 115200;
    cfg.uart.dataBits = 8;
    cfg.uart.parity = "none";
    cfg.uart.stopBits = 1;

    // Network
    cfg.telnetPort = 2323;
    cfg.httpPort = 8080;

    // WiFi — set via WIFI_SSID and WIFI_PASS environment variables
    cfg.wifi.ssid = WIFI_SSID;
    cfg.wifi.password = WIFI_PASS;
    cfg.wifi.maxRetries = 10;

    // Display
    cfg.displayTimeoutMs = 60000;

    // Auto-responder rules
    cfg.commandSequence.lineEnding = "\n";
    cfg.commandSequence.promptTimeoutMs = 0; // 0 = wait forever

    PromptRule loginRule;
    loginRule.trigger = "login:";
    loginRule.response = "root";
    cfg.commandSequence.rules.push_back(loginRule);

    PostCommand postCmd;
    postCmd.command = "journalctl -f -o short-monotonic";
    postCmd.expectedPrompt = "#";
    postCmd.delayMs = 1000;
    cfg.commandSequence.postCommands.push_back(postCmd);

    return cfg;
}

// ============================================================
// Global instances
// ============================================================

static M5UartModule uart;
static M5TelnetServer telnet;
static M5HttpServer http;
static M5DisplayModule display;
static M5WiFiModule wifi;
static M5StorageModule storage; // 20MB max file, 100 files

static AutoResponder* autoResponder = nullptr;
static BridgeController* controller = nullptr;

static uint32_t lastTickMs = 0;
static BridgeConfig savedConfig;
static bool wasIdle = false;

void setup() {
    // Disable brownout detector to prevent spurious resets on power dips
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    M5.begin(true, true, true); // LCD=true, SD=true, Serial=true
    M5.Power.begin();

    Serial.println("UART-Telnet Bridge starting...");

    // Initialize SD card storage (optional — continues without it)
    if (storage.init()) {
        Serial.println("SD card storage ready.");
    } else {
        Serial.println("SD card not available — logging to SD disabled.");
    }

    // Let display show SD status
    display.setStorage(&storage);

    BridgeConfig config = makeConfig();
    savedConfig = config;

    // AutoResponder writes to UART
    autoResponder = new AutoResponder([](const uint8_t* data, size_t len) {
        uart.write(data, len);
    });

    controller = new BridgeController(uart, telnet, http, display, wifi, *autoResponder,
                                       storage.isAvailable() ? &storage : nullptr);

    if (!controller->startup(config)) {
        M5.Lcd.fillScreen(RED);
        M5.Lcd.setTextColor(WHITE, RED);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(10, 100);
        M5.Lcd.println("STARTUP FAILED");
        Serial.println("Bridge startup failed!");
        while (true) { delay(1000); }
    }

    lastTickMs = millis();
    Serial.println("Bridge running.");

    // Send a newline to provoke a prompt from the device.
    // If already logged in, this shows "#" or "$". If logged out, shows "login:".
    delay(500);
    const char* nl = "\n";
    uart.write(reinterpret_cast<const uint8_t*>(nl), 1);
}

void loop() {
    uint32_t now = millis();
    uint32_t elapsed = now - lastTickMs;
    lastTickMs = now;

    // Poll all modules (Arduino is single-threaded, no background threads)
    uart.poll();
    telnet.poll();
    http.poll();
    display.pollButtons();
    storage.poll();

    // Tick the controller
    controller->tick(elapsed);

    // If AutoResponder went IDLE (login+postcommands completed or device rebooted),
    // re-arm it once so it catches the next login: prompt
    if (autoResponder->getState() == AutoResponder::State::IDLE &&
        !savedConfig.commandSequence.rules.empty() && !wasIdle) {
        wasIdle = true;
        autoResponder->setConfig(savedConfig.commandSequence);
        Serial.println("AutoResponder re-armed, waiting for login...");
    } else if (autoResponder->getState() != AutoResponder::State::IDLE) {
        wasIdle = false;
    }

    // Yield to WiFi/system tasks to prevent watchdog reset
    delay(10);
}
