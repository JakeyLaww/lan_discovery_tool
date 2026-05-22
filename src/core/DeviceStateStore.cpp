#include "core/DeviceStateStore.hpp"
#include <sstream>

std::string DeviceStateStore::make_key(const std::string& src_ip,
                                       const ResourceRecordView& rec) const {
    std::ostringstream oss;
    oss << src_ip << '|' << rec.owner_name << '|' << rec.type << '|' << rec.rdata_text;
    return oss.str();
}

bool DeviceStateStore::update_and_changed(const DiscoveryEvent& ev) {
    bool changed = false;
    for (const auto& rec : ev.records) {
        const std::string key = make_key(ev.src_ip, rec);
        const auto it = last_seen_.find(key);
        if (it == last_seen_.end()) {
            last_seen_.emplace(key, rec.rdata_text);
            changed = true;
        }
    }
    return changed;
}
