/**
 * @file LinuxUartModule.cpp
 * @brief Linux UART implementation using POSIX termios.
 */

#include "LinuxUartModule.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
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

    fd_ = ::open(config.portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        return false;
    }

    struct termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    speed_t speed = toBaudConstant(config.baudRate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Data bits
    tty.c_cflag &= ~CSIZE;
    switch (config.dataBits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        default: tty.c_cflag |= CS8; break;
    }

    // Parity
    if (config.parity == "even") {
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
    } else if (config.parity == "odd") {
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD;
    } else {
        tty.c_cflag &= ~PARENB;
    }

    // Stop bits
    if (config.stopBits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // 100ms read timeout

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
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
