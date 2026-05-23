#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <string>
#include <unordered_set>
#include <vector>

/**
 * @brief Tracks discovery record keys already emitted and suppresses duplicates.
 *
 * Keys records by record_store_key(src_ip, record). Used to suppress
 * duplicate console output for identical src_ip|owner|type|rdata.
 */
class DeviceStateStore {
public:
    /**
     * @return Records from the batch not yet seen (updates store).
     */
    std::vector<ResourceRecordView> filter_unseen(const std::string& src_ip,
                                                  const std::vector<ResourceRecordView>& records);

private:
    std::unordered_set<std::string> seen_keys_;
};
