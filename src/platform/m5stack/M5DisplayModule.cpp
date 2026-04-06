/**
 * @file M5DisplayModule.cpp
 * @brief M5Stack Basic v1.0 display implementation.
 */

#include "M5DisplayModule.h"
#include <M5Stack.h>

void M5DisplayModule::init() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("UART-Telnet Bridge");
    M5.Lcd.println("Initializing...");
    lastActivityMs_ = millis();
}

void M5DisplayModule::update(const DisplayStatus& status) {
    // Check backlight timeout
    if (backlightOn_ && (millis() - lastActivityMs_) > BACKLIGHT_TIMEOUT_MS) {
        setBacklight(false);
    }

    // Only redraw if backlight is on
    if (!backlightOn_) return;

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);

    // WiFi status
    if (status.wifiConnected) {
        M5.Lcd.setTextColor(GREEN, BLACK);
        M5.Lcd.println("WiFi: Connected");
        M5.Lcd.printf("RSSI: %d dBm\n", status.signalStrengthDbm);
        M5.Lcd.printf("IP: %s\n", status.ipAddress.c_str());
    } else {
        M5.Lcd.setTextColor(RED, BLACK);
        M5.Lcd.println("WiFi: Disconnected");
    }

    // Separator
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.println("----------------");

    // UART throughput
    M5.Lcd.setTextColor(CYAN, BLACK);
    M5.Lcd.printf("RX: %u B/s\n", status.uartRxBytesPerSec);
    M5.Lcd.printf("TX: %u B/s\n", status.uartTxBytesPerSec);
}

void M5DisplayModule::setBacklight(bool on) {
    backlightOn_ = on;
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
        onButtonPress();
    }
}
