#include "discovery/DeviceRegistry.hpp"
#include "discovery/MdnsNames.hpp"
#include "discovery/RecordLabels.hpp"
#include "mdns/DnsTypes.hpp"
#include <sstream>

namespace {

bool insert_if_new(std::set<std::string>& set, const std::string& value) {
    if (value.empty()) return false;
    return set.insert(value).second;
}

bool add_service_type(std::set<std::string>& service_types, const std::string& name) {
    return insert_if_new(service_types, MdnsNames::service_type_from_name(name));
}

std::string join_set(const std::set<std::string>& values, size_t max_count) {
    std::ostringstream oss;
    size_t i = 0;
    for (const auto& v : values) {
        if (i >= max_count) break;
        if (i > 0) oss << ", ";
        oss << v;
        ++i;
    }
    if (values.size() > max_count) {
        if (max_count > 0) oss << ", ";
        oss << "... (" << values.size() << " total)";
    }
    return oss.str();
}

} // namespace

bool DeviceRegistry::merge_records(DeviceSnapshot& snap, const DiscoveryEvent& ev) {
    bool profile_changed = false;

    for (const auto& rec : ev.records) {
        if (rec.observed_at_ms > snap.last_seen_ms) {
            snap.last_seen_ms = rec.observed_at_ms;
        }

        if (rec.owner_name == MdnsNames::kDnsSdMetaService && rec.type == DnsType::PTR &&
            MdnsNames::is_service_type_name(rec.rdata_text)) {
            if (insert_if_new(snap.service_types, rec.rdata_text)) profile_changed = true;
        }

        if (MdnsNames::is_service_type_name(rec.owner_name) && rec.type == DnsType::PTR &&
            MdnsNames::is_instance_name(rec.rdata_text)) {
            if (insert_if_new(snap.instances, rec.rdata_text)) profile_changed = true;
            if (add_service_type(snap.service_types, rec.rdata_text)) profile_changed = true;
        }

        if (rec.type == DnsType::SRV || rec.type == DnsType::TXT) {
            if (add_service_type(snap.service_types, rec.owner_name)) profile_changed = true;
        }

        const std::string host = host_label(rec);
        if (!host.empty() && rec.type == DnsType::SRV) {
            if (snap.mdns_host != host) {
                snap.mdns_host = host;
                profile_changed = true;
            }
        } else if ((rec.type == DnsType::A || rec.type == DnsType::AAAA) &&
                   MdnsNames::ends_with(rec.owner_name, ".local") && snap.mdns_host.empty()) {
            snap.mdns_host = rec.owner_name;
            profile_changed = true;
        }
    }

    return profile_changed;
}

bool DeviceRegistry::apply(const DiscoveryEvent& ev) {
    if (ev.src_ip.empty() || ev.records.empty()) return false;

    DeviceSnapshot& snap = devices_[ev.src_ip];
    if (snap.src_ip.empty()) {
        snap.src_ip = ev.src_ip;
    }

    return merge_records(snap, ev);
}

const DeviceSnapshot* DeviceRegistry::find(const std::string& src_ip) const {
    const auto it = devices_.find(src_ip);
    if (it == devices_.end()) return nullptr;
    return &it->second;
}

std::string DeviceRegistry::format_summary(const std::string& src_ip) const {
    const DeviceSnapshot* snap = find(src_ip);
    if (!snap) return {};

    std::ostringstream oss;
    oss << "device src=" << snap->src_ip
        << " last_seen=" << format_timestamp_iso(snap->last_seen_ms);
    if (!snap->mdns_host.empty()) {
        oss << " host=" << snap->mdns_host;
    }

    if (!snap->service_types.empty()) {
        oss << "\n  services: " << join_set(snap->service_types, snap->service_types.size());
    }
    if (!snap->instances.empty()) {
        oss << "\n  instances: " << join_set(snap->instances, kMaxInstancesInSummary);
    }

    return oss.str();
}
