/**
 * @file LinuxStorageModule.cpp
 * @brief Linux file-based log storage with rotation.
 */

#include "LinuxStorageModule.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>

namespace fs = std::filesystem;

LinuxStorageModule::LinuxStorageModule(std::string logDir,
                                       size_t maxFileSize,
                                       int maxFiles)
    : logDir_(std::move(logDir))
    , maxFileSize_(maxFileSize)
    , maxFiles_(maxFiles) {}

LinuxStorageModule::~LinuxStorageModule() {
    flush();
}

bool LinuxStorageModule::init() {
    available_ = false;

    // Create log directory if it does not exist
    std::error_code ec;
    fs::create_directories(logDir_, ec);
    if (ec) {
        std::cerr << "[Storage] Cannot create log directory \"" << logDir_
                  << "\": " << ec.message() << std::endl;
        return false;
    }

    // Scan existing log files to find the next write index
    cachedFileCount_ = 0;
    currentFileIndex_ = 0;

    for (int i = 0; i < maxFiles_; ++i) {
        std::string path = makeFilePath(i);
        std::error_code statEc;
        auto fileSize = fs::file_size(path, statEc);
        if (statEc) {
            // File doesn't exist — use this slot
            currentFileIndex_ = i;
            break;
        }
        ++cachedFileCount_;
        if (fileSize < maxFileSize_) {
            // Resume writing to this partially-filled file
            currentFileIndex_ = i;
            break;
        }
        if (i == maxFiles_ - 1) {
            // All slots full — wrap around to 0
            currentFileIndex_ = 0;
        }
    }

    openNextFile();
    available_ = currentFile_.is_open();
    if (available_) {
        std::cout << "[Storage] Log storage ready, writing to "
                  << makeFilePath(currentFileIndex_)
                  << " (" << cachedFileCount_ << " existing files)" << std::endl;
    }
    return available_;
}

void LinuxStorageModule::write(const uint8_t* data, size_t length) {
    if (!available_ || !currentFile_.is_open()) return;

    currentFile_.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(length));
    if (currentFile_.fail()) {
        std::cerr << "[Storage] Write failed — storage marked unavailable" << std::endl;
        available_ = false;
        currentFile_.close();
        return;
    }
    currentFileSize_ += length;
    writesSinceFlush_ += length;

    if (writesSinceFlush_ >= FLUSH_THRESHOLD) {
        writesSinceFlush_ = 0;
        currentFile_.flush();
    }

    rotateIfNeeded();
}

void LinuxStorageModule::flush() {
    if (currentFile_.is_open()) {
        currentFile_.flush();
    }
}

bool LinuxStorageModule::isAvailable() const {
    return available_;
}

int LinuxStorageModule::getFileCount() const {
    return cachedFileCount_;
}

std::string LinuxStorageModule::getCurrentFileName() const {
    if (!available_) return "";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "log%03d.txt", currentFileIndex_);
    return buf;
}

void LinuxStorageModule::poll() {
    // No hot-insert/eject on Linux — nothing to do.
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void LinuxStorageModule::openNextFile() {
    if (currentFile_.is_open()) {
        currentFile_.close();
    }

    std::string path = makeFilePath(currentFileIndex_);

    // Determine existing size (to resume or overwrite)
    std::error_code ec;
    auto existingSize = fs::file_size(path, ec);
    if (!ec && existingSize >= maxFileSize_) {
        // Overwrite the full file
        std::error_code removeEc;
        fs::remove(path, removeEc);
        currentFileSize_ = 0;
    } else if (!ec) {
        currentFileSize_ = existingSize;
    } else {
        currentFileSize_ = 0;
    }

    // Open in append mode so we don't discard partial files
    currentFile_.open(path, std::ios::binary | std::ios::app);
    if (!currentFile_.is_open()) {
        std::cerr << "[Storage] Cannot open log file \"" << path
                  << "\": " << std::strerror(errno) << std::endl;
    }
}

void LinuxStorageModule::rotateIfNeeded() {
    if (currentFileSize_ < maxFileSize_) return;

    std::cout << "[Storage] Rotating: " << makeFilePath(currentFileIndex_)
              << " full (" << currentFileSize_ << " bytes)" << std::endl;

    currentFile_.close();
    currentFileIndex_ = (currentFileIndex_ + 1) % maxFiles_;
    cachedFileCount_ = std::min(cachedFileCount_ + 1, maxFiles_);

    // Remove destination file if it already exists (oldest log)
    std::string path = makeFilePath(currentFileIndex_);
    std::error_code ec;
    fs::remove(path, ec);

    currentFileSize_ = 0;
    currentFile_.open(path, std::ios::binary | std::ios::trunc);
    if (currentFile_.is_open()) {
        std::cout << "[Storage] Now writing to " << path << std::endl;
    }
}

std::string LinuxStorageModule::makeFilePath(int index) const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "/log%03d.txt", index);
    return logDir_ + buf;
}
