#!/bin/bash
set -e

if [ -z "$WIFI_SSID" ] || [ -z "$WIFI_PASS" ]; then
    echo "Error: Set WIFI_SSID and WIFI_PASS environment variables first."
    echo "  export WIFI_SSID=\"YourNetwork\""
    echo "  export WIFI_PASS=\"YourPassword\""
    exit 1
fi

# Ensure symlinks exist
ln -sf ../../src/core/AutoResponder.cpp platformio/src/AutoResponder.cpp
ln -sf ../../src/core/BridgeController.cpp platformio/src/BridgeController.cpp
ln -sf ../../src/platform/m5stack/M5UartModule.cpp platformio/src/M5UartModule.cpp
ln -sf ../../src/platform/m5stack/M5TelnetServer.cpp platformio/src/M5TelnetServer.cpp
ln -sf ../../src/platform/m5stack/M5HttpServer.cpp platformio/src/M5HttpServer.cpp
ln -sf ../../src/platform/m5stack/M5DisplayModule.cpp platformio/src/M5DisplayModule.cpp
ln -sf ../../src/platform/m5stack/M5WiFiModule.cpp platformio/src/M5WiFiModule.cpp
ln -sf ../../src/platform/m5stack/M5ConfigParser.cpp platformio/src/M5ConfigParser.cpp

echo "=== Building M5Stack firmware ==="
echo "WiFi SSID: $WIFI_SSID"

cd platformio
pio run

echo "=== Flashing ==="
pio run -t upload

echo "=== Done. Opening serial monitor (Ctrl+C to exit) ==="
pio device monitor
