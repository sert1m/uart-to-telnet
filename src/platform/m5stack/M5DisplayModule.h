/**
 * @file M5DisplayModule.h
 * @brief M5Stack Basic v1.0 display implementation using M5Stack.h.
 */

#ifndef PLATFORM_M5STACK_DISPLAY_MODULE_H
#define PLATFORM_M5STACK_DISPLAY_MODULE_H

#include "interfaces/DisplayModule.h"
#include <cstdint>

/**
 * @brief M5Stack display rendering WiFi status, IP, and UART throughput.
 *
 * Manages backlight timeout (off after 60s inactivity) and button A press
 * to wake the display.
 */
class M5DisplayModule : public DisplayModule {
public:
    void init() override;
    void update(const DisplayStatus& status) override;
    void setBacklight(bool on) override;
    void onButtonPress() override;

    /// Call from loop() to check button A press.
    void pollButtons();

private:
    bool backlightOn_ = true;
    uint32_t lastActivityMs_ = 0;
    static constexpr uint32_t BACKLIGHT_TIMEOUT_MS = 60000;
};

#endif // PLATFORM_M5STACK_DISPLAY_MODULE_H
