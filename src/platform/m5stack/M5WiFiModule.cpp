/**
 * @file M5WiFiModule.cpp
 * @brief M5Stack Arduino WiFi implementation with retry logic.
 */

#include "M5WiFiModule.h"
#include <Arduino.h>

bool M5WiFiModule::connect(const WiFiConfig& config) {
    config_ = config;

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < config.maxRetries) {
        delay(5000);
        retries++;
        Serial.printf("WiFi retry %d/%d...\n", retries, config.maxRetries);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    Serial.println("WiFi connection failed, restarting cycle...");
    // Restart cycle — caller can retry
    return false;
}

void M5WiFiModule::disconnect() {
    WiFi.disconnect(true);
}

bool M5WiFiModule::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

std::string M5WiFiModule::getIpAddress() const {
    if (WiFi.status() != WL_CONNECTED) return "";
    return WiFi.localIP().toString().c_str();
}

int M5WiFiModule::getSignalStrengthDbm() const {
    if (WiFi.status() != WL_CONNECTED) return 0;
    return WiFi.RSSI();
}
