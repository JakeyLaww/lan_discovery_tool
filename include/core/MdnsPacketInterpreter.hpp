#pragma once

#include "Logger.hpp"
#include "discovery/DiscoveryTypes.hpp"
#include "mdns/DnsProtocol.hpp"
#include "mdns/Parser.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

class MdnsPacketInterpreter {
public:
    static constexpr uint16_t kDnsQrResponse = DnsProtocol::kFlagsResponse;

    explicit MdnsPacketInterpreter(std::shared_ptr<Logger> logger);

    void set_verbose(bool verbose) { verbose_ = verbose; }

    static bool is_dns_response(const MdnsParsedMessage& msg);

    MdnsParsedMessage decode_packet(const std::vector<uint8_t>& buffer);

    DiscoveryEvent create_discovery_event(const std::string& src_ip,
                                         const std::vector<uint8_t>& packet,
                                         const MdnsParsedMessage& parsed_packet);

    void log_ignored_query(const std::string& src_ip);

    void log_undisplayable_response(const std::string& src_ip, const MdnsParsedMessage& parsed_packet);

    void log_packet_summary(const std::string& src_ip,
                           const std::vector<uint8_t>& buffer,
                           const MdnsParsedMessage& parsed_packet);

private:
    std::shared_ptr<Logger> logger;
    bool verbose_{false};
};
