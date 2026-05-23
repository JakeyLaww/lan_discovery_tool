#include "core/DeviceStateStore.hpp"
#include "discovery/DiscoveryRecord.hpp"

std::vector<ResourceRecordView> DeviceStateStore::filter_new(
    const std::string& src_ip,
    const std::vector<ResourceRecordView>& records) {
    std::vector<ResourceRecordView> out;
    out.reserve(records.size());
    for (const auto& rec : records) {
        const std::string key = record_store_key(src_ip, rec);
        if (last_seen_.find(key) != last_seen_.end()) continue;
        last_seen_.emplace(key, RecordState{rec.rdata_text, rec.observed_at_ms});
        out.push_back(rec);
    }
    return out;
}
