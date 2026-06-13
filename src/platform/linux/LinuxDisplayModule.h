/**
 * @file LinuxDisplayModule.h
 * @brief Linux implementation of DisplayModule.
 *
 * Prints status to stdout and writes a human-readable status snapshot to
 * /tmp/uart-bridge-status.txt so external tools can inspect bridge state.
 */

#ifndef PLATFORM_LINUX_DISPLAY_MODULE_H
#define PLATFORM_LINUX_DISPLAY_MODULE_H

#include "interfaces/DisplayModule.h"
#include "interfaces/StorageModule.h"

#include <string>

/**
 * @brief Linux display that logs status to console and a /tmp status file.
 *
 * On each update() call the module writes a fixed-format status block to
 * /tmp/uart-bridge-status.txt, mirroring the information shown on the
 * M5Stack LCD (IP, throughput, uptime, log file info).
 */
class LinuxDisplayModule : public DisplayModule {
public:
    explicit LinuxDisplayModule(const std::string& statusFile = "/tmp/uart-bridge-status.txt");

    void init() override;
    void update(const DisplayStatus& status) override;
    void setBacklight(bool on) override;   ///< No-op on Linux.
    void onButtonPress() override;         ///< No-op on Linux.

    /// Set optional storage module reference for log file status display.
    void setStorage(StorageModule* storage) { storage_ = storage; }

private:
    std::string statusFile_;
    StorageModule* storage_ = nullptr;
    uint32_t uptimeSec_ = 0;  ///< Accumulated uptime in seconds.
};

#endif // PLATFORM_LINUX_DISPLAY_MODULE_H
