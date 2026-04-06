/**
 * @file M5StackDisplayModule.cpp
 * @brief M5 Stack implementation of DisplayModule.
 *
 * Skeleton implementation with TODO comments for actual M5 Stack display API calls.
 * This file is NOT compiled on Linux — it is only included when PLATFORM=esp32.
 */

#include "M5StackDisplayModule.h"

// TODO: #include <M5Stack.h>
// TODO: #include "esp_log.h"

M5StackDisplayModule::M5StackDisplayModule() = default;

void M5StackDisplayModule::init() {
    // TODO: Initialize M5 Stack display via M5.begin() or M5.Lcd.begin()
    // TODO: Set text size, color, and initial screen layout
    // TODO: Set lastActivityMs_ = millis() (or esp_timer_get_time() / 1000)
    // TODO: Turn backlight on initially
}

void M5StackDisplayModule::update(const DisplayStatus& status) {
    // TODO: Clear the display or specific regions
    // TODO: Render WiFi status:
    //   - If status.wifiConnected: show "WiFi: Connected"
    //   - Else: show "WiFi: Disconnected"
    // TODO: Render signal strength: show status.signalStrengthDbm in dBm
    // TODO: Render IP address: show status.ipAddress
    // TODO: Render UART throughput:
    //   - "RX: <status.uartRxBytesPerSec> B/s"
    //   - "TX: <status.uartTxBytesPerSec> B/s"

    // TODO: Backlight timeout check:
    //   uint32_t now = millis();
    //   if (backlightOn_ && (now - lastActivityMs_) > BACKLIGHT_TIMEOUT_MS) {
    //       setBacklight(false);
    //   }
}

void M5StackDisplayModule::setBacklight(bool on) {
    backlightOn_ = on;

    // TODO: Control M5 Stack backlight:
    //   M5.Lcd.setBrightness(on ? 255 : 0);
    //   or use M5.Power.setLcdBrightness()
}

void M5StackDisplayModule::onButtonPress() {
    // TODO: Reset the backlight timeout:
    //   lastActivityMs_ = millis();
    // TODO: If backlight is off, turn it back on:
    //   if (!backlightOn_) {
    //       setBacklight(true);
    //   }
}
