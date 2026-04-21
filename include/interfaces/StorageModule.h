/**
 * @file StorageModule.h
 * @brief Abstract interface for log file storage.
 *
 * Platform-specific implementations handle writing UART data to
 * persistent storage (SD card, filesystem, etc.) with file rotation.
 */

#ifndef INTERFACES_STORAGE_MODULE_H
#define INTERFACES_STORAGE_MODULE_H

#include <cstdint>
#include <cstddef>
#include <string>

/**
 * @brief Abstract interface for storing UART log data to persistent storage.
 */
class StorageModule {
public:
    virtual ~StorageModule() = default;

    /**
     * @brief Initialize the storage (mount SD card, create directories, etc.).
     * @return true if storage is available and ready, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Write data to the current log file.
     * @param data Pointer to the byte buffer to write.
     * @param length Number of bytes to write.
     *
     * Implementations should handle file rotation automatically when
     * the current file exceeds the configured max size.
     */
    virtual void write(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Flush any buffered data to disk.
     */
    virtual void flush() = 0;

    /**
     * @brief Check if storage is available and mounted.
     * @return true if storage is ready for writing.
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief Get the number of log files currently on storage.
     * @return Number of log files, or 0 if storage is unavailable.
     */
    virtual int getFileCount() const = 0;

    /**
     * @brief Get the current log file name.
     * @return File name string, or empty if unavailable.
     */
    virtual std::string getCurrentFileName() const = 0;

    /**
     * @brief Poll for hot insert/eject of storage media.
     *
     * Call periodically from the main loop. Detects card insertion
     * (re-initializes) and ejection (marks unavailable).
     */
    virtual void poll() = 0;
};

#endif // INTERFACES_STORAGE_MODULE_H
