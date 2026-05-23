#include "NetworkSocket.hpp"
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define THROW_ERRNO(msg) throw std::system_error(errno, std::generic_category(), std::string(msg) + " : " + std::string(std::strerror(errno)))

/**
 * @brief Construct a new NetworkSocket object.
 */
NetworkSocket::NetworkSocket() : sock_fd(-1) {}

/**
 * @brief Destroy the NetworkSocket object and close the socket if open.
 */
NetworkSocket::~NetworkSocket() {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
}

/**
 * @brief Helper to safely close socket and throw on error.
 */
void NetworkSocket::cleanup_and_throw(const char* err_msg) {
    int err = errno;
    close(sock_fd);
    sock_fd = -1;
    errno = err;
    THROW_ERRNO(err_msg);
}

/**
 * @brief Bind the socket to a multicast group and port.
 *
 * @param port Port number in host byte order to bind to.
 * @param multicast_ip Multicast IPv4 address (dotted decimal).
 */
void NetworkSocket::bind_multicast(uint16_t port, const std::string& multicast_ip,
                                 const std::string& interface_ipv4) {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        THROW_ERRNO("socket()");
    }

    int reuse = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        cleanup_and_throw("setsockopt(SO_REUSEADDR)");
    }

#ifdef SO_REUSEPORT
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, (char *)&reuse, sizeof(reuse)) < 0) {
        cleanup_and_throw("setsockopt(SO_REUSEPORT)");
    }
#endif

    struct sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, reinterpret_cast<struct sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
        cleanup_and_throw("bind()");
    }

    struct in_addr mcast;
    if (inet_pton(AF_INET, multicast_ip.c_str(), &mcast) != 1) {
        int err = errno;
        close(sock_fd);
        sock_fd = -1;
        errno = err;
        throw std::invalid_argument("Invalid multicast IP: " + multicast_ip);
    }

    struct in_addr send_iface{};
    send_iface.s_addr = htonl(INADDR_ANY);
    if (!interface_ipv4.empty()) {
        if (inet_pton(AF_INET, interface_ipv4.c_str(), &send_iface) != 1) {
            throw std::invalid_argument("Invalid interface IPv4: " + interface_ipv4);
        }
        if (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &send_iface, sizeof(send_iface)) < 0) {
            cleanup_and_throw("setsockopt(IP_MULTICAST_IF)");
        }
    }

    // Receive on all local interfaces (INADDR_ANY). Pinning imr_interface to one NIC
    // breaks multicast ingress on WSL2 mirrored networking; egress uses send_iface above.
    struct in_addr recv_membership{};
    recv_membership.s_addr = htonl(INADDR_ANY);

    struct ip_mreq mreq{};
    mreq.imr_multiaddr = mcast;
    mreq.imr_interface = recv_membership;
    if (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        cleanup_and_throw("setsockopt(IP_ADD_MEMBERSHIP)");
    }

    int loop = 0;
    if (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
        cleanup_and_throw("setsockopt(IP_MULTICAST_LOOP)");
    }
}

/**
 * @brief Send a UDP packet to the specified destination.
 *
 * @param data Payload bytes to send.
 * @param dest_ip Destination IPv4 address (dotted decimal).
 * @param dest_port Destination port in host byte order.
 * @return ssize_t Number of bytes sent.
 */
ssize_t NetworkSocket::send_packet(const std::vector<uint8_t>& data, const std::string& dest_ip, uint16_t dest_port) {
    if (sock_fd < 0) {
        throw std::system_error(ENOTSOCK, std::generic_category(), "send_packet: socket not initialized");
    }

    struct sockaddr_in dest_addr;
    std::memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);

    if (inet_pton(AF_INET, dest_ip.c_str(), &dest_addr.sin_addr) != 1) {
        throw std::invalid_argument("Invalid destination IP: " + dest_ip);
    }

    ssize_t sent = sendto(sock_fd, data.data(), data.size(), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        THROW_ERRNO("sendto()");
    }
    return sent;
}

/**
 * @brief Receive a UDP packet into the provided buffer and return the sender IP.
 *
 * @param buffer Vector to be filled with the received payload (resized to actual length).
 * @param sender_ip Output string containing the sender IPv4 address in dotted form.
 * @return ssize_t Number of bytes received.
 */
ssize_t NetworkSocket::receive_packet(std::vector<uint8_t>& buffer, std::string& sender_ip) {
    if (sock_fd < 0) {
        throw std::system_error(ENOTSOCK, std::generic_category(), "receive_packet: socket not initialized");
    }
    
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // Resize buffer to maximum UDP payload size to avoid truncation
    buffer.resize(65535);

    ssize_t bytes_read = recvfrom(sock_fd, buffer.data(), buffer.size(), 0,
                                  (struct sockaddr*)&sender_addr, &sender_len);

    if (bytes_read < 0) {
        THROW_ERRNO("recvfrom()");
    }

    buffer.resize(static_cast<size_t>(bytes_read)); // Shrink to actual size

    // Convert sender address to text safely
    char ipbuf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sender_addr.sin_addr, ipbuf, sizeof(ipbuf)) == nullptr) {
        // If conversion fails, still return bytes but provide empty sender_ip
        sender_ip.clear();
    } else {
        sender_ip = ipbuf;
    }

    return bytes_read;
}

/**
 * @brief Set the receive timeout for the socket using SO_RCVTIMEO.
 *
 * @param milliseconds Timeout in milliseconds; 0 disables timeout (blocking).
 */
void NetworkSocket::set_receive_timeout(uint32_t milliseconds) {
    if (sock_fd < 0) {
        throw std::system_error(ENOTSOCK, std::generic_category(), "set_receive_timeout: socket not initialized");
    }
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        THROW_ERRNO("setsockopt(SO_RCVTIMEO)");
    }
}

#undef THROW_ERRNO