#include "core/MdnsPacketInterpreter.hpp"
#include "discovery/DiscoveryRecord.hpp"
#include "discovery/RecordFilter.hpp"
#include "mdns/MdnsMessageDecoder.hpp"
#include "util/HexDump.hpp"
#include <chrono>
#include <sstream>
#include <unordered_set>

namespace {

void append_records(std::vector<ResourceRecordView>& out,
                    const std::vector<MdnsResourceRecord>& rrs,
                    const uint8_t* packet,
                    size_t packet_len,
                    uint64_t observed_at_ms,
                    std::unordered_set<std::string>& seen) {
    for (const auto& rr : rrs) {
        ResourceRecordView view = make_record_view(rr, packet, packet_len, observed_at_ms);
        if (!is_displayable_record(view)) continue;
        const std::string key = record_content_key(view);
        if (!seen.insert(key).second) continue;
        out.push_back(std::move(view));
    }
}

} // namespace

MdnsPacketInterpreter::MdnsPacketInterpreter(std::shared_ptr<Logger> logger_)
    : logger(std::move(logger_)) {}

bool MdnsPacketInterpreter::is_dns_response(const MdnsParsedMessage& msg) {
    return (msg.header.flags & kDnsQrResponse) != 0;
}

MdnsParsedMessage MdnsPacketInterpreter::decode_packet(const std::vector<uint8_t>& buffer) {
    MdnsMessageDecoder decoder(buffer.data(), buffer.size());
    return decoder.decode();
}

DiscoveryEvent MdnsPacketInterpreter::create_discovery_event(
    const std::string& src_ip,
    const std::vector<uint8_t>& packet,
    const MdnsParsedMessage& parsed_packet) {
    DiscoveryEvent ev;
    ev.timestamp_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    ev.src_ip = src_ip;

    const uint8_t* wire = packet.data();
    const size_t wire_len = packet.size();
    std::unordered_set<std::string> seen;
    append_records(ev.records, parsed_packet.answers, wire, wire_len, ev.timestamp_ms, seen);
    append_records(ev.records, parsed_packet.authorities, wire, wire_len, ev.timestamp_ms, seen);
    append_records(ev.records, parsed_packet.additionals, wire, wire_len, ev.timestamp_ms, seen);

    return ev;
}

void MdnsPacketInterpreter::log_ignored_query(const std::string& src_ip) {
    logger->debug("ignored query from " + src_ip);
}

void MdnsPacketInterpreter::log_undisplayable_response(const std::string& src_ip,
                                                       const MdnsParsedMessage& parsed_packet) {
    std::ostringstream oss;
    oss << "response from " << src_ip << " with " << parsed_packet.header.answer_rrs
        << " answers, 0 displayable records";
    logger->debug(oss.str());
}

void MdnsPacketInterpreter::log_packet_summary(const std::string& src_ip,
                                               const std::vector<uint8_t>& buffer,
                                               const MdnsParsedMessage& parsed_packet) {
    if (!verbose_) return;

    std::string dump_label = "Packet from " + src_ip;
    HexDump::print(dump_label, buffer, logger);

    std::ostringstream summary;
    summary << "Parsed: " << src_ip << " "
            << parsed_packet.header.questions << " Q, "
            << parsed_packet.header.answer_rrs << " A, "
            << parsed_packet.header.authority_rrs << " Auth, "
            << parsed_packet.header.additional_rrs << " Add";
    logger->debug(summary.str());
}
