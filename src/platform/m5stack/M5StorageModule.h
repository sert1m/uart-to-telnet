/**
 * @file M5StorageModule.h
 * @brief M5Stack SD card storage with file rotation.
 */

#ifndef PLATFORM_M5STACK_STORAGE_MODULE_H
#define PLATFORM_M5STACK_STORAGE_MODULE_H

#include "interfaces/StorageModule.h"
#include <SD.h>
#include <cstdint>
#include <string>

/**
 * @brief M5Stack SD card log storage.
 *
 * Writes UART data to /logs/logNNN.txt on the SD card.
 * Rotates files when current file exceeds maxFileSize.
 * Keeps up to maxFiles, deleting the oldest when rotating.
 */
class M5StorageModule : public StorageModule {
public:
    M5StorageModule(size_t maxFileSize = 20 * 1024 * 1024,
                    int maxFiles = 100);
    ~M5StorageModule() override = default;

    bool init() override;
    void write(const uint8_t* data, size_t length) override;
    void flush() override;
    bool isAvailable() const override;
    int getFileCount() const override;
    std::string getCurrentFileName() const override;
    void poll() override;

private:
    void openNextFile();
    void rotateIfNeeded();
    String makeFilePath(int index) const;

    size_t maxFileSize_;
    int maxFiles_;
    int currentFileIndex_ = 0;
    size_t currentFileSize_ = 0;
    File currentFile_;
    bool available_ = false;
    int cachedFileCount_ = 0;
    uint32_t lastPollMs_ = 0;
    size_t writesSinceFlush_ = 0;
    static constexpr uint32_t POLL_INTERVAL_MS = 5000;
    static constexpr size_t FLUSH_THRESHOLD = 4096; // flush every 4KB
};

#endif // PLATFORM_M5STACK_STORAGE_MODULE_H
