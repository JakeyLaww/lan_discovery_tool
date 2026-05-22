#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <string>
#include <unordered_map>

/**
 * @brief Tracks last-seen discovery records and reports when state changes.
 *
 * Keys records by (src_ip, owner_name, type, rdata_text). Used to suppress
 * duplicate console output and to gate Phase 2 POST-on-change behavior.
 */
class DeviceStateStore {
public:
    /**
     * @return true if any record in the event is new or changed since last seen.
     */
    bool update_and_changed(const DiscoveryEvent& ev);

    void clear();

private:
    std::string make_key(const std::string& src_ip, const ResourceRecordView& rec) const;

    std::unordered_map<std::string, std::string> last_seen_;
};
