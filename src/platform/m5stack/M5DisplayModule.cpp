/**
 * @file M5DisplayModule.cpp
 * @brief M5Stack Basic v1.0 display implementation.
 *
 * Uses partial redraws — only clears and repaints individual text fields
 * instead of fillScreen() to avoid flickering.
 */

#include "M5DisplayModule.h"
#include <M5Stack.h>
#include "Version.h"

// Layout constants (y positions for each row at textSize=2, 16px per row)
static constexpr int Y_TITLE    = 0;
static constexpr int Y_WIFI     = 20;
static constexpr int Y_RSSI     = 40;
static constexpr int Y_IP       = 60;
static constexpr int Y_SEP      = 80;
static constexpr int Y_RX       = 100;
static constexpr int Y_TX       = 120;
static constexpr int Y_BAT      = 140;
static constexpr int Y_UPTIME   = 160;
static constexpr int Y_SD       = 180;
static constexpr int ROW_H      = 20;
static constexpr int SCREEN_W   = 320;

/// Clear a single row and print new text.
static void drawRow(int y, uint16_t fg, const char* text) {
    M5.Lcd.fillRect(0, y, SCREEN_W, ROW_H, BLACK);
    M5.Lcd.setTextColor(fg, BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.print(text);
}

void M5DisplayModule::init() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    char title[64];
    snprintf(title, sizeof(title), "UART-Telnet v%s", SW_VERSION);
    drawRow(Y_TITLE, GREEN, title);
    drawRow(Y_SEP, WHITE, "----------------");
    lastActivityMs_ = millis();
}

void M5DisplayModule::update(const DisplayStatus& status) {
    // Check backlight timeout
    if (backlightOn_ && (millis() - lastActivityMs_) > BACKLIGHT_TIMEOUT_MS) {
        setBacklight(false);
    }

    if (!backlightOn_) return;

    M5.Lcd.setTextSize(2);

    // WiFi status
    char buf[64];
    if (status.wifiConnected) {
        drawRow(Y_WIFI, GREEN, "WiFi: Connected");
        snprintf(buf, sizeof(buf), "RSSI: %d dBm    ", status.signalStrengthDbm);
        drawRow(Y_RSSI, GREEN, buf);
        snprintf(buf, sizeof(buf), "IP: %s    ", status.ipAddress.c_str());
        drawRow(Y_IP, GREEN, buf);
    } else {
        drawRow(Y_WIFI, RED, "WiFi: Disconnected");
        drawRow(Y_RSSI, RED, "");
        drawRow(Y_IP, RED, "");
    }

    // UART throughput
    snprintf(buf, sizeof(buf), "RX: %u B/s    ", status.uartRxBytesPerSec);
    drawRow(Y_RX, CYAN, buf);
    snprintf(buf, sizeof(buf), "TX: %u B/s    ", status.uartTxBytesPerSec);
    drawRow(Y_TX, CYAN, buf);

    // Battery level
    int8_t batLevel = M5.Power.getBatteryLevel();
    uint16_t batColor = (batLevel > 50) ? GREEN : (batLevel > 20) ? YELLOW : RED;
    snprintf(buf, sizeof(buf), "Bat: %d%%    ", batLevel);
    drawRow(Y_BAT, batColor, buf);

    // Uptime
    uint32_t uptimeSec = millis() / 1000;
    snprintf(buf, sizeof(buf), "Up: %us    ", uptimeSec);
    drawRow(Y_UPTIME, WHITE, buf);

    // SD card status
    if (storage_ && storage_->isAvailable()) {
        std::string fname = storage_->getCurrentFileName();
        snprintf(buf, sizeof(buf), "SD:%d %s    ", storage_->getFileCount(), fname.c_str());
        drawRow(Y_SD, GREEN, buf);
    } else {
        drawRow(Y_SD, DARKGREY, "SD: not available");
    }
}
void M5DisplayModule::setBacklight(bool on) {
    if (backlightOn_ == on) return; // no change
    backlightOn_ = on;
    Serial.printf("[Display] Backlight %s\n", on ? "ON" : "OFF");
    M5.Lcd.setBrightness(on ? 200 : 0);
}

void M5DisplayModule::onButtonPress() {
    lastActivityMs_ = millis();
    if (!backlightOn_) {
        setBacklight(true);
    }
}

void M5DisplayModule::pollButtons() {
    M5.update();
    if (M5.BtnA.wasPressed()) {
        Serial.println("[Display] Button A pressed — waking backlight");
        onButtonPress();
    }
}
