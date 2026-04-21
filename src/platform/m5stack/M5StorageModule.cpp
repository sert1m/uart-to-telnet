/**
 * @file M5StorageModule.cpp
 * @brief M5Stack SD card log storage with file rotation and hot insert/eject.
 */

#include "M5StorageModule.h"
#include <Arduino.h>

M5StorageModule::M5StorageModule(size_t maxFileSize, int maxFiles)
    : maxFileSize_(maxFileSize), maxFiles_(maxFiles) {}

bool M5StorageModule::init() {
    if (!SD.begin(4)) { // GPIO 4 = SD card CS on M5Stack Basic
        Serial.println("[Storage] SD card not found");
        available_ = false;
        return false;
    }

    if (!SD.exists("/logs")) {
        SD.mkdir("/logs");
    }

    // Find the next available file index
    cachedFileCount_ = 0;
    for (int i = 0; i < maxFiles_; i++) {
        String path = makeFilePath(i);
        if (!SD.exists(path)) {
            currentFileIndex_ = i;
            break;
        }
        cachedFileCount_++;
        if (i == maxFiles_ - 1) {
            currentFileIndex_ = 0;
        } else {
            File f = SD.open(path, FILE_READ);
            if (f) {
                size_t sz = f.size();
                f.close();
                if (sz < maxFileSize_) {
                    currentFileIndex_ = i;
                    break;
                }
            }
        }
    }

    openNextFile();
    available_ = (bool)currentFile_;
    if (available_) {
        Serial.printf("[Storage] SD ready, writing to %s (%d files)\n",
                      makeFilePath(currentFileIndex_).c_str(), cachedFileCount_);
    }
    return available_;
}

void M5StorageModule::write(const uint8_t* data, size_t length) {
    if (!available_ || !currentFile_) return;

    size_t written = currentFile_.write(data, length);
    currentFileSize_ += written;

    // Flush periodically and check for ejection
    writesSinceFlush_ += written;
    if (writesSinceFlush_ >= FLUSH_THRESHOLD) {
        writesSinceFlush_ = 0;
        currentFile_.flush();
        // After flush, if file is no longer valid the position resets
        if (!currentFile_) {
            Serial.println("[Storage] SD card ejected (write failed)");
            available_ = false;
            cachedFileCount_ = 0;
            SD.end();
            return;
        }
    }

    rotateIfNeeded();
}

void M5StorageModule::flush() {
    if (currentFile_) {
        currentFile_.flush();
    }
}

bool M5StorageModule::isAvailable() const {
    return available_;
}

int M5StorageModule::getFileCount() const {
    return cachedFileCount_;
}

std::string M5StorageModule::getCurrentFileName() const {
    if (!available_) return "";
    char buf[32];
    snprintf(buf, sizeof(buf), "log%03d.txt", currentFileIndex_);
    return buf;
}

void M5StorageModule::poll() {
    uint32_t now = millis();
    if (now - lastPollMs_ < POLL_INTERVAL_MS) return;
    lastPollMs_ = now;

    if (available_) {
        // Detect ejection: try to stat the logs directory
        // This is a lightweight SPI operation that fails fast if card is gone
        File dir = SD.open("/logs");
        if (!dir) {
            Serial.println("[Storage] SD card ejected (directory check failed)");
            currentFile_.close();
            available_ = false;
            cachedFileCount_ = 0;
            SD.end();
        } else {
            dir.close();
        }
    } else {
        // Try to re-init if card was inserted
        init();
    }
}

void M5StorageModule::openNextFile() {
    if (currentFile_) {
        currentFile_.close();
    }

    String path = makeFilePath(currentFileIndex_);

    if (SD.exists(path)) {
        File f = SD.open(path, FILE_READ);
        if (f) {
            currentFileSize_ = f.size();
            f.close();
            if (currentFileSize_ >= maxFileSize_) {
                SD.remove(path);
                currentFileSize_ = 0;
            }
        }
    } else {
        currentFileSize_ = 0;
    }

    currentFile_ = SD.open(path, FILE_APPEND);
}

void M5StorageModule::rotateIfNeeded() {
    if (currentFileSize_ < maxFileSize_) return;

    Serial.printf("[Storage] Rotating: %s full (%u bytes)\n",
                  makeFilePath(currentFileIndex_).c_str(),
                  (unsigned)currentFileSize_);

    currentFile_.close();
    currentFileIndex_ = (currentFileIndex_ + 1) % maxFiles_;
    cachedFileCount_ = min(cachedFileCount_ + 1, maxFiles_);

    String path = makeFilePath(currentFileIndex_);
    if (SD.exists(path)) {
        SD.remove(path);
    }

    currentFileSize_ = 0;
    currentFile_ = SD.open(path, FILE_APPEND);

    if (currentFile_) {
        Serial.printf("[Storage] Now writing to %s\n", path.c_str());
    }
}

String M5StorageModule::makeFilePath(int index) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "/logs/log%03d.txt", index);
    return String(buf);
}
