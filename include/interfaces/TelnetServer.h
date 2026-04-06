/**
 * @file TelnetServer.h
 * @brief Abstract interface for the Telnet server.
 *
 * Provides a platform-independent contract for accepting Telnet connections,
 * broadcasting UART data to clients, and receiving commands from clients.
 */

#ifndef INTERFACES_TELNET_SERVER_H
#define INTERFACES_TELNET_SERVER_H

#include <cstdint>
#include <functional>

/**
 * @brief Abstract interface for the Telnet server.
 *
 * Provides a platform-independent contract for accepting Telnet connections,
 * broadcasting UART data to clients, and receiving commands from clients.
 */
class TelnetServer {
public:
    virtual ~TelnetServer() = default;

    /**
     * @brief Start listening for incoming TCP connections.
     * @param port TCP port number to listen on.
     * @return true if the server started successfully, false if the port is in use.
     * @post Server accepts incoming TCP connections on @p port.
     */
    virtual bool start(uint16_t port) = 0;

    /**
     * @brief Stop the server and disconnect all connected clients.
     */
    virtual void stop() = 0;

    /**
     * @brief Broadcast data to all connected Telnet clients.
     * @param data Pointer to the byte buffer to send.
     * @param length Number of bytes to broadcast.
     */
    virtual void broadcast(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Register a callback invoked when a connected client sends data.
     * @param callback Function receiving the client ID, data pointer, and data length.
     */
    virtual void setOnClientDataCallback(
        std::function<void(int clientId, const uint8_t*, size_t)> callback) = 0;

    /**
     * @brief Register a callback invoked when a client connects or disconnects.
     * @param callback Function receiving the client ID and a boolean (true = connected, false = disconnected).
     */
    virtual void setOnClientEventCallback(
        std::function<void(int clientId, bool connected)> callback) = 0;

    /**
     * @brief Get the number of currently connected clients.
     * @return Number of active Telnet client connections.
     */
    virtual int clientCount() const = 0;
};

#endif // INTERFACES_TELNET_SERVER_H
