#include "core/DeviceStateStore.hpp"
#include "discovery/DiscoveryRecord.hpp"

std::vector<ResourceRecordView> DeviceStateStore::filter_unseen(
    const std::string& src_ip,
    const std::vector<ResourceRecordView>& records) {
    std::vector<ResourceRecordView> out;
    out.reserve(records.size());
    for (const auto& rec : records) {
        const std::string key = record_store_key(src_ip, rec);
        if (!seen_keys_.insert(key).second) continue;
        out.push_back(rec);
    }
    return out;
}
