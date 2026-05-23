#include "mdns/RDataFormatter.hpp"
#include "mdns/DnsTypes.hpp"
#include "MDNSDefinitions.hpp"
#include <sstream>
#include <iomanip>

std::string ARecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    if (rdata.size() != 4) {
        return "(malformed A record)";
    }
    std::ostringstream oss;
    oss << static_cast<int>(rdata[0]) << "."
        << static_cast<int>(rdata[1]) << "."
        << static_cast<int>(rdata[2]) << "."
        << static_cast<int>(rdata[3]);
    return oss.str();
}

std::string AAAARecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    if (rdata.size() != 16) {
        return "(malformed AAAA record)";
    }
    std::ostringstream oss;
    oss << std::hex;
    for (size_t i = 0; i < 16; i += 2) {
        if (i > 0) oss << ":";
        uint16_t group = (static_cast<uint16_t>(rdata[i]) << 8) | rdata[i + 1];
        oss << group;
    }
    return oss.str();
}

std::string PTRRecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    if (rdata.empty()) {
        return "(empty PTR record)";
    }
    if (!ctx.message || ctx.message_len == 0) {
        return "(malformed PTR record)";
    }
    try {
        auto [ptr_name, _] = MDNS::parse_qname(ctx.message, ctx.message_len, ctx.rdata_offset);
        return ptr_name;
    } catch (...) {
        return "(malformed PTR record)";
    }
}

std::string SRVRecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    if (rdata.size() < 6) {
        return "(malformed SRV record)";
    }
    std::ostringstream oss;
    uint16_t priority = MDNS::read_u16_be(rdata.data());
    uint16_t weight = MDNS::read_u16_be(rdata.data() + 2);
    uint16_t port = MDNS::read_u16_be(rdata.data() + 4);
    oss << "priority=" << priority << " weight=" << weight << " port=" << port;

    if (rdata.size() > 6 && ctx.message && ctx.message_len > 0) {
        try {
            const size_t target_offset = ctx.rdata_offset + 6;
            auto [target, _] = MDNS::parse_qname(ctx.message, ctx.message_len, target_offset);
            oss << " target=" << target;
        } catch (...) {
            oss << " (malformed target name)";
        }
    }
    return oss.str();
}

std::string TXTRecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    std::ostringstream oss;
    size_t pos = 0;
    bool first = true;
    while (pos < rdata.size()) {
        uint8_t str_len = rdata[pos];
        ++pos;
        if (pos + str_len > rdata.size()) break;
        if (!first) oss << " ";
        oss << std::string(reinterpret_cast<const char*>(rdata.data() + pos), str_len);
        pos += str_len;
        first = false;
    }
    return oss.str();
}

std::string GenericRecordFormatter::format(const RDataFormatContext& ctx) const {
    const auto& rdata = *ctx.rdata;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < rdata.size() && i < 32; ++i) {
        oss << std::setw(2) << static_cast<int>(rdata[i]);
    }
    if (rdata.size() > 32) oss << "...";
    return oss.str();
}

RDataFormatterPtr get_rdata_formatter(uint16_t type) {
    switch (type) {
        case DnsType::A:       return std::make_shared<ARecordFormatter>();
        case DnsType::AAAA:    return std::make_shared<AAAARecordFormatter>();
        case DnsType::PTR:     return std::make_shared<PTRRecordFormatter>();
        case DnsType::SRV:     return std::make_shared<SRVRecordFormatter>();
        case DnsType::TXT:     return std::make_shared<TXTRecordFormatter>();
        default:               return std::make_shared<GenericRecordFormatter>();
    }
}
