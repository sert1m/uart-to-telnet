/**
 * @file LinuxUartModule.cpp
 * @brief Linux UART implementation using POSIX termios.
 */

#include "LinuxUartModule.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t toBaudConstant(uint32_t baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return B115200;
    }
}

} // namespace

LinuxUartModule::LinuxUartModule() = default;

LinuxUartModule::~LinuxUartModule() {
    close();
}

bool LinuxUartModule::open(const UartConfig& config) {
    config_ = config;

    fd_ = ::open(config.portName.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) {
        std::cerr << "LinuxUartModule: open(\"" << config.portName
                  << "\") failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Build termios from scratch — do NOT inherit kernel defaults via tcgetattr,
    // as leftover cooked-mode flags cause garbled data.
    struct termios tty{};
    memset(&tty, 0, sizeof(tty));

    speed_t speed = toBaudConstant(config.baudRate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // c_cflag: data bits, parity, stop bits, enable receiver, ignore modem lines
    tty.c_cflag = CLOCAL | CREAD;
    switch (config.dataBits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        default: tty.c_cflag |= CS8; break;
    }
    if (config.parity == "even") {
        tty.c_cflag |= PARENB;
    } else if (config.parity == "odd") {
        tty.c_cflag |= PARENB | PARODD;
    }
    if (config.stopBits == 2) {
        tty.c_cflag |= CSTOPB;
    }

    // c_iflag: no input processing at all (raw)
    tty.c_iflag = 0;

    // c_oflag: no output processing (raw)
    tty.c_oflag = 0;

    // c_lflag: no line processing (non-canonical, no echo, no signals)
    tty.c_lflag = 0;

    // Blocking read: return after at least 1 byte, with 100ms inter-byte timeout
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    // Flush any stale data in the driver buffers
    tcflush(fd_, TCIOFLUSH);

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        std::cerr << "LinuxUartModule: tcsetattr failed: " << strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Verify settings took effect (log warning if mismatch, but don't fail —
    // some USB-serial drivers report back slightly different flag bits)
    struct termios verify{};
    tcgetattr(fd_, &verify);
    if (cfgetispeed(&verify) != speed) {
        std::cerr << "LinuxUartModule: warning — baud rate may not have been set correctly"
                  << std::endl;
    }

    // Start read thread if callback is set
    if (onDataCallback_) {
        running_ = true;
        readThread_ = std::thread(&LinuxUartModule::readLoop, this);
    }

    return true;
}

void LinuxUartModule::close() {
    running_ = false;
    if (readThread_.joinable()) {
        readThread_.join();
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

int LinuxUartModule::write(const uint8_t* data, size_t length) {
    if (fd_ < 0) {
        return -1;
    }
    ssize_t written = ::write(fd_, data, length);
    return static_cast<int>(written);
}

void LinuxUartModule::setOnDataCallback(std::function<void(const uint8_t*, size_t)> callback) {
    onDataCallback_ = std::move(callback);
    // If port is already open, start the read thread now
    if (fd_ >= 0 && !running_) {
        running_ = true;
        readThread_ = std::thread(&LinuxUartModule::readLoop, this);
    }
}

bool LinuxUartModule::isOpen() const {
    return fd_ >= 0;
}

bool LinuxUartModule::reopen() {
    close();
    return open(config_);
}

void LinuxUartModule::readLoop() {
    uint8_t buf[256];
    while (running_) {
        ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n > 0 && onDataCallback_) {
            onDataCallback_(buf, static_cast<size_t>(n));
        }
    }
}
