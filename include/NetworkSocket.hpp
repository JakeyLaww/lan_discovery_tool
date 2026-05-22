#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <netinet/in.h>
#include <system_error>

/**
 * @brief Simple RAII UDP socket wrapper.
 *
 * Methods in this class throw std::system_error on failure. The wrapper
 * manages the underlying socket descriptor and provides utilities for
 * multicast bind, send/receive, and receive timeouts.
 */
class NetworkSocket {
private:
    int sock_fd;

    /**
     * @brief Helper to safely close socket and clean up on error.
     *
     * @param err_msg Error message for exception.
     */
    void cleanup_and_throw(const char* err_msg);

public:
    /**
     * @brief Construct a new NetworkSocket object.
     */
    NetworkSocket();

    /**
     * @brief Destroy the NetworkSocket object and close the socket if open.
     */
    ~NetworkSocket();

    // Prevent copying because this manages a raw OS resource (RAII principle)
    NetworkSocket(const NetworkSocket&) = delete;
    NetworkSocket& operator=(const NetworkSocket&) = delete;

    /** @param interface_ipv4 Optional local IPv4 for multicast egress (empty = INADDR_ANY). */
    void bind_multicast(uint16_t port, const std::string& multicast_ip,
                        const std::string& interface_ipv4 = "");

    /**
     * @brief Send a UDP packet to the specified destination.
     *
     * @param data Byte vector containing the payload to send.
     * @param dest_ip Destination IPv4 address (dotted decimal).
     * @param dest_port Destination UDP port (host byte order).
     */
    ssize_t send_packet(const std::vector<uint8_t>& data, const std::string& dest_ip, uint16_t dest_port);

    /**
     * @brief Receive a UDP packet into the provided buffer and capture sender IP.
     *
     * The buffer will be resized to the received payload length on success.
     *
     * @param buffer Vector to receive payload into.
     * @param sender_ip Output parameter set to the sender's dotted IPv4 address.
     * @return ssize_t Number of bytes received.
     */
    ssize_t receive_packet(std::vector<uint8_t>& buffer, std::string& sender_ip);

    /**
     * @brief Set the receive timeout for the underlying socket.
     *
     * @param milliseconds Timeout in milliseconds (0 disables timeout and sets blocking mode).
     */
    void set_receive_timeout(uint32_t milliseconds);
};