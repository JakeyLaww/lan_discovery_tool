#include "discovery/RecordLabels.hpp"
#include "mdns/DnsTypes.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string format_timestamp_iso(uint64_t timestamp_ms) {
    const std::time_t sec = static_cast<std::time_t>(timestamp_ms / 1000);
    const int ms = static_cast<int>(timestamp_ms % 1000);
    std::tm tm{};
    gmtime_r(&sec, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms << 'Z';
    return oss.str();
}

namespace {

bool ends_with(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool looks_like_service_name(const std::string& name) {
    return ends_with(name, "._tcp.local") || ends_with(name, "._udp.local");
}

std::string extract_after_prefix(const std::string& text, const std::string& prefix) {
    const auto pos = text.find(prefix);
    if (pos == std::string::npos) return {};
    return text.substr(pos + prefix.size());
}

} // namespace

std::string service_label(const ResourceRecordView& rec) {
    if (rec.type == DnsType::PTR && looks_like_service_name(rec.rdata_text)) {
        return rec.rdata_text;
    }
    if (rec.type == DnsType::SRV) {
        const std::string target = extract_after_prefix(rec.rdata_text, "target=");
        if (!target.empty()) return target;
    }
    if (looks_like_service_name(rec.owner_name)) {
        return rec.owner_name;
    }
    return {};
}

std::string host_label(const ResourceRecordView& rec) {
    if (rec.type == DnsType::SRV) {
        const std::string target = extract_after_prefix(rec.rdata_text, "target=");
        if (!target.empty()) return target;
    }
    if (rec.type == DnsType::A || rec.type == DnsType::AAAA) {
        if (ends_with(rec.owner_name, ".local")) {
            return rec.owner_name;
        }
        return rec.owner_name + " (" + rec.rdata_text + ")";
    }
    if (rec.type == DnsType::TXT && ends_with(rec.owner_name, ".local")) {
        return rec.owner_name;
    }
    return {};
}
