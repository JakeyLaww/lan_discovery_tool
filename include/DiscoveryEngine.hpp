#pragma once
#include "NetworkSocket.hpp"
#include <memory>
#include <vector>
#include <string>

/**
 * @brief High-level discovery engine that broadcasts mDNS queries and listens for responses.
 */
class DiscoveryEngine {
private:
    std::unique_ptr<NetworkSocket> socket;

    /**
     * @brief Utility to print a hex dump of received data.
     *
     * @param desc Short description to include in the dump header.
     * @param data Byte vector to display.
     */
    void hex_dump(const std::string& desc, const std::vector<uint8_t>& data);

public:
    /**
     * @brief Construct a new DiscoveryEngine instance.
     */
    DiscoveryEngine();

    /**
     * @brief Start the engine by binding sockets and preparing to send/receive.
     *
     * @return true if initialization succeeded; false otherwise.
     */
    bool start();

    /**
     * @brief Send a service discovery mDNS query.
     */
    void broadcast_query();

    /**
     * @brief Enter the receive loop and parse incoming responses.
     *
     * This method currently runs until an error occurs; higher-level code
     * should arrange for process termination or a future stop() hook.
     */
    void listen_and_parse();
};