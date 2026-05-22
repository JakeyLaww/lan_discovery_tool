#include "discovery/DiscoveryTypes.hpp"
#include <sstream>

std::string json_escape(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string to_json(const DiscoveryEvent& ev) {
    std::ostringstream oss;
    oss << "{\"timestamp_ms\":" << ev.timestamp_ms
        << ",\"src_ip\":\"" << json_escape(ev.src_ip) << "\""
        << ",\"records\":[";
    for (size_t i = 0; i < ev.records.size(); ++i) {
        if (i > 0) oss << ',';
        const auto& r = ev.records[i];
        oss << "{\"owner_name\":\"" << json_escape(r.owner_name) << "\""
            << ",\"type\":" << r.type
            << ",\"rdata_text\":\"" << json_escape(r.rdata_text) << "\"}";
    }
    oss << "]}";
    return oss.str();
}
