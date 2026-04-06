/**
 * @file M5TelnetServer.cpp
 * @brief M5Stack Arduino Telnet server using WiFiServer/WiFiClient.
 */

#include "M5TelnetServer.h"

M5TelnetServer::M5TelnetServer() {
    for (auto& slot : clients_) {
        slot.active = false;
        slot.id = 0;
    }
}

bool M5TelnetServer::start(uint16_t port) {
    port_ = port;
    server_ = new WiFiServer(port);
    server_->begin();
    server_->setNoDelay(true);
    running_ = true;
    return true;
}

void M5TelnetServer::stop() {
    running_ = false;
    for (auto& slot : clients_) {
        if (slot.active) {
            slot.client.stop();
            if (onClientEvent_) onClientEvent_(slot.id, false);
            slot.active = false;
        }
    }
    if (server_) {
        server_->stop();
        delete server_;
        server_ = nullptr;
    }
}

void M5TelnetServer::broadcast(const uint8_t* data, size_t length) {
    for (auto& slot : clients_) {
        if (slot.active && slot.client.connected()) {
            slot.client.write(data, length);
        }
    }
}

void M5TelnetServer::setOnClientDataCallback(
    std::function<void(int, const uint8_t*, size_t)> callback) {
    onClientData_ = std::move(callback);
}

void M5TelnetServer::setOnClientEventCallback(
    std::function<void(int, bool)> callback) {
    onClientEvent_ = std::move(callback);
}

int M5TelnetServer::clientCount() const {
    int count = 0;
    for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients_[i].active) count++;
    }
    return count;
}

void M5TelnetServer::poll() {
    if (!running_ || !server_) return;

    // Accept new clients
    if (server_->hasClient()) {
        WiFiClient newClient = server_->available();
        if (newClient) {
            // Find a free slot
            int freeSlot = -1;
            for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
                if (!clients_[i].active || !clients_[i].client.connected()) {
                    if (clients_[i].active) {
                        // Slot was active but client disconnected
                        if (onClientEvent_) onClientEvent_(clients_[i].id, false);
                    }
                    freeSlot = i;
                    break;
                }
            }
            if (freeSlot >= 0) {
                clients_[freeSlot].client = newClient;
                clients_[freeSlot].id = nextClientId_++;
                clients_[freeSlot].active = true;
                clients_[freeSlot].client.setNoDelay(true);
                if (onClientEvent_) onClientEvent_(clients_[freeSlot].id, true);
            } else {
                newClient.stop(); // No room
            }
        }
    }

    // Read data from connected clients
    uint8_t buf[256];
    for (auto& slot : clients_) {
        if (!slot.active) continue;

        if (!slot.client.connected()) {
            slot.active = false;
            if (onClientEvent_) onClientEvent_(slot.id, false);
            continue;
        }

        int avail = slot.client.available();
        if (avail > 0) {
            size_t toRead = (avail > (int)sizeof(buf)) ? sizeof(buf) : (size_t)avail;
            size_t n = slot.client.readBytes(buf, toRead);
            if (n > 0 && onClientData_) {
                onClientData_(slot.id, buf, n);
            }
        }
    }
}
