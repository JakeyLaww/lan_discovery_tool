#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct RecordState {
    std::string rdata_text;
    uint64_t observed_at_ms = 0;
};

/**
 * @brief Tracks last-seen discovery records and reports when state changes.
 *
 * Keys records by record_store_key(src_ip, record). Used to suppress
 * duplicate console output and to gate Phase 2 POST-on-change behavior.
 */
class DeviceStateStore {
public:
    /**
     * @return Records from the batch that are new since last seen (updates store).
     */
    std::vector<ResourceRecordView> filter_new(const std::string& src_ip,
                                               const std::vector<ResourceRecordView>& records);

private:
    std::unordered_map<std::string, RecordState> last_seen_;
};
