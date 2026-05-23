#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"

namespace MdnsNames {

bool ends_with(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool has_mdns_service_suffix(const std::string& name) {
    return ends_with(name, "._tcp.local") || ends_with(name, "._udp.local");
}

namespace {

bool is_dns_sd_meta_service(const std::string& name) {
    return name == kDnsSdMetaService;
}

} // namespace

int label_count(const std::string& name) {
    if (name.empty()) return 0;
    int count = 1;
    for (char c : name) {
        if (c == '.') ++count;
    }
    return count;
}

bool is_service_type_name(const std::string& name) {
    if (!has_mdns_service_suffix(name)) return false;
    if (is_dns_sd_meta_service(name)) return false;
    return label_count(name) == 3;
}

bool is_instance_name(const std::string& name) {
    if (!has_mdns_service_suffix(name)) return false;
    if (is_dns_sd_meta_service(name)) return false;
    return label_count(name) >= 4;
}

namespace {

bool contains_arpa(const std::string& name) {
    return name.find(".arpa") != std::string::npos;
}

bool is_bonjour_owner(const std::string& owner) {
    return is_instance_name(owner) || is_service_type_name(owner);
}

} // namespace

std::string service_type_from_name(const std::string& name) {
    if (is_service_type_name(name)) return name;
    if (!is_instance_name(name)) return {};

    size_t transport_pos = name.rfind("._tcp.local");
    if (transport_pos == std::string::npos) {
        transport_pos = name.rfind("._udp.local");
    }
    if (transport_pos == std::string::npos || transport_pos == 0) return {};

    const size_t dot_before_service = name.rfind('.', transport_pos - 1);
    if (dot_before_service == std::string::npos) return {};
    return name.substr(dot_before_service + 1);
}

bool is_audit_relevant_record(const ResourceRecordView& rec) {
    switch (rec.type) {
        case DnsType::PTR:
            if (contains_arpa(rec.owner_name) || contains_arpa(rec.rdata_text)) return false;
            if (rec.owner_name == kDnsSdMetaService &&
                is_service_type_name(rec.rdata_text)) {
                return true;
            }
            return is_service_type_name(rec.owner_name) && is_instance_name(rec.rdata_text);

        case DnsType::SRV:
        case DnsType::TXT:
            return is_bonjour_owner(rec.owner_name);

        case DnsType::A:
        case DnsType::AAAA:
            return ends_with(rec.owner_name, ".local");

        default:
            return false;
    }
}

} // namespace MdnsNames
