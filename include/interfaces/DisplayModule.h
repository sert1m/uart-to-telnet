/**
 * @file DisplayModule.h
 * @brief Abstract interface for the status display module.
 *
 * On M5 Stack, renders WiFi status, signal strength, IP address, and UART
 * throughput on the built-in screen with backlight timeout management.
 * On Linux, prints status to the console. Backlight methods are no-ops on Linux.
 */

#ifndef INTERFACES_DISPLAY_MODULE_H
#define INTERFACES_DISPLAY_MODULE_H

#include "core/DataModels.h"

/**
 * @brief Abstract interface for the status display module.
 *
 * On M5 Stack, renders WiFi status, signal strength, IP address, and UART
 * throughput on the built-in screen with backlight timeout management.
 * On Linux, prints status to the console. Backlight methods are no-ops on Linux.
 */
class DisplayModule {
public:
    virtual ~DisplayModule() = default;

    /**
     * @brief Initialize the display hardware or console output.
     */
    virtual void init() = 0;

    /**
     * @brief Update the displayed status information.
     * @param status Current bridge status including WiFi state and UART throughput.
     */
    virtual void update(const DisplayStatus& status) = 0;

    /**
     * @brief Turn the display backlight on or off.
     * @param on true to enable backlight, false to disable.
     * @note On Linux this is a no-op.
     */
    virtual void setBacklight(bool on) = 0;

    /**
     * @brief Notify the display that a physical button was pressed.
     *
     * Resets the backlight timeout counter. On Linux this is a no-op.
     */
    virtual void onButtonPress() = 0;
};

#endif // INTERFACES_DISPLAY_MODULE_H
