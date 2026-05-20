#include "DiscoveryEngine.hpp"
#include "MDNSDefinitions.hpp"
#include <iostream>
#include <iomanip>
#include <system_error>

/**
 * @brief Construct a new DiscoveryEngine instance.
 */
DiscoveryEngine::DiscoveryEngine() {
    socket = std::make_unique<NetworkSocket>();
}

/**
 * @brief Start the engine by binding to the mDNS multicast group.
 *
 * @return true on success, false on failure.
 */
bool DiscoveryEngine::start() {
    try {
        socket->bind_multicast(MDNS::PORT, MDNS::MULTICAST_GROUP);
    } catch (const std::system_error& ex) {
        std::cerr << "Failed to bind to mDNS multicast group: " << ex.what() << "\n";
        return false;
    } catch (const std::exception& ex) {
        std::cerr << "Failed to bind to mDNS multicast group: " << ex.what() << "\n";
        return false;
    }
    std::cout << "Engine active. Bound to " << MDNS::MULTICAST_GROUP << ":" << MDNS::PORT << "\n";
    return true;
}

/**
 * @brief Broadcast the prepared mDNS service discovery query.
 */
void DiscoveryEngine::broadcast_query() {
    // Phase 2 Payload: Query for _services._dns-sd._udp.local
    std::vector<uint8_t> query = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x09, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
        0x07, '_', 'd', 'n', 's', '-', 's', 'd',
        0x04, '_', 'u', 'd', 'p',
        0x05, 'l', 'o', 'c', 'a', 'l',
        0x00, 0x00, 0x0c, 0x00, 0x01
    };

    if (socket->send_packet(query, MDNS::MULTICAST_GROUP, MDNS::PORT) > 0) {
        std::cout << "Broadcasted service discovery query.\n";
    }
}

/**
 * @brief Listen for incoming UDP responses and attempt to parse DNS headers.
 *
 * The loop runs until a network error occurs. Parsing errors for individual
 * packets are logged but do not terminate the loop.
 */
void DiscoveryEngine::listen_and_parse() {
    std::cout << "Listening for network responses...\n\n";
    std::vector<uint8_t> buffer;
    std::string sender_ip;

    while (true) {
        try {
            ssize_t n = socket->receive_packet(buffer, sender_ip);
            if (n > 0) {
                std::string log_desc = "Packet from " + sender_ip;
                hex_dump(log_desc, buffer);

                // Attempt safe parse of DNS header
                try {
                    auto hdr = MDNS::parse_dns_header(buffer.data(), buffer.size());
                    std::cout << "Parsed DNS header: tid=" << hdr.transaction_id
                              << " flags=0x" << std::hex << hdr.flags << std::dec
                              << " questions=" << hdr.questions
                              << " answers=" << hdr.answer_rrs << "\n";
                } catch (const std::exception& ex) {
                    std::cerr << "Failed to parse DNS header: " << ex.what() << "\n";
                }
            }
        } catch (const std::system_error& ex) {
            std::cerr << "Network error while receiving: " << ex.what() << "\n";
            break;
        }
    }
}

/**
 * @brief Print a hex dump of the provided byte vector.
 *
 * This helper preserves and restores std::cout's formatting state.
 */
void DiscoveryEngine::hex_dump(const std::string& desc, const std::vector<uint8_t>& data) {
    std::ios oldState(nullptr);
    oldState.copyfmt(std::cout);
    std::cout << "--- " << desc << " (" << std::dec << data.size() << " bytes) ---\n";
    for (size_t i = 0; i < data.size(); i++) {
        if (i % 16 == 0) std::cout << std::hex << std::setw(4) << std::setfill('0') << i << "  ";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
        if (i % 16 == 15 || i == data.size() - 1) std::cout << "\n";
    }
    std::cout << "\n";
    std::cout.copyfmt(oldState);
}