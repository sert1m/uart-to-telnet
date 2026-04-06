/**
 * @file LinuxDisplayModule.h
 * @brief Linux implementation of DisplayModule (console output).
 */

#ifndef PLATFORM_LINUX_DISPLAY_MODULE_H
#define PLATFORM_LINUX_DISPLAY_MODULE_H

#include "interfaces/DisplayModule.h"

/**
 * @brief Linux display implementation that prints status to stdout.
 *
 * Prints IP address and UART RX/TX throughput to the console.
 * Backlight and button press methods are no-ops on Linux.
 */
class LinuxDisplayModule : public DisplayModule {
public:
    void init() override;
    void update(const DisplayStatus& status) override;
    void setBacklight(bool on) override;
    void onButtonPress() override;
};

#endif // PLATFORM_LINUX_DISPLAY_MODULE_H
