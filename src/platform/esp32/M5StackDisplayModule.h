/**
 * @file M5StackDisplayModule.h
 * @brief M5 Stack implementation of DisplayModule using M5 Stack display API.
 */

#ifndef PLATFORM_ESP32_M5STACK_DISPLAY_MODULE_H
#define PLATFORM_ESP32_M5STACK_DISPLAY_MODULE_H

#include "interfaces/DisplayModule.h"

#include <cstdint>

/**
 * @brief M5 Stack display implementation.
 *
 * Renders WiFi status, signal strength, IP address, and UART RX/TX
 * throughput on the M5 Stack built-in screen. Manages backlight timeout
 * (turns off after 1 minute of inactivity) and button press handling
 * (resets the backlight timeout on press).
 */
class M5StackDisplayModule : public DisplayModule {
public:
    M5StackDisplayModule();
    ~M5StackDisplayModule() override = default;

    void init() override;
    void update(const DisplayStatus& status) override;
    void setBacklight(bool on) override;
    void onButtonPress() override;

private:
    bool backlightOn_ = true;
    uint32_t lastActivityMs_ = 0;

    // TODO: Backlight timeout tracking:
    //   - Store the timestamp of the last button press (lastActivityMs_)
    //   - In update(), check if (currentTimeMs - lastActivityMs_) > displayTimeoutMs
    //   - If timeout exceeded, call setBacklight(false)

    // TODO: Button press handling:
    //   - In onButtonPress(), reset lastActivityMs_ to current time
    //   - If backlight is off, call setBacklight(true)

    static constexpr uint32_t BACKLIGHT_TIMEOUT_MS = 60000; ///< 1 minute backlight timeout.
};

#endif // PLATFORM_ESP32_M5STACK_DISPLAY_MODULE_H
