/**
 * @file LinuxStorageModule.h
 * @brief Linux file-based log storage with file rotation.
 *
 * Mirrors the behaviour of M5StorageModule but writes to a configurable
 * directory on the host filesystem instead of an SD card.
 */

#ifndef PLATFORM_LINUX_STORAGE_MODULE_H
#define PLATFORM_LINUX_STORAGE_MODULE_H

#include "interfaces/StorageModule.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

/**
 * @brief Linux log storage that writes to rotating files in a directory.
 *
 * Files are named logNNN.txt (e.g., log000.txt, log001.txt …).
 * A new file is opened when the current one reaches maxFileSize.
 * At most maxFiles log files are kept; the oldest is overwritten on rotation.
 */
class LinuxStorageModule : public StorageModule {
public:
    /**
     * @brief Construct with storage parameters.
     * @param logDir      Directory path where log files are stored.
     * @param maxFileSize Maximum size per log file in bytes (default: 20 MB).
     * @param maxFiles    Maximum number of log files to keep (default: 100).
     */
    explicit LinuxStorageModule(std::string logDir,
                                size_t maxFileSize = 20 * 1024 * 1024,
                                int maxFiles = 100);
    ~LinuxStorageModule() override;

    bool init() override;
    void write(const uint8_t* data, size_t length) override;
    void flush() override;
    bool isAvailable() const override;
    int getFileCount() const override;
    std::string getCurrentFileName() const override;
    void poll() override;  ///< No-op on Linux (no hot-insert media).

private:
    void openNextFile();
    void rotateIfNeeded();
    std::string makeFilePath(int index) const;

    std::string logDir_;
    size_t maxFileSize_;
    int maxFiles_;

    int currentFileIndex_ = 0;
    size_t currentFileSize_ = 0;
    std::ofstream currentFile_;
    bool available_ = false;
    int cachedFileCount_ = 0;
    size_t writesSinceFlush_ = 0;

    static constexpr size_t FLUSH_THRESHOLD = 4096;  ///< Flush every 4 KB.
};

#endif // PLATFORM_LINUX_STORAGE_MODULE_H
