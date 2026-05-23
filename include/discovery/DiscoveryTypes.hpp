#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ResourceRecordView {
    std::string owner_name;
    uint16_t type;
    std::string rdata_text;
    uint32_t ttl = 0;
    uint64_t observed_at_ms = 0;
};

struct DiscoveryEvent {
    uint64_t timestamp_ms;
    std::string src_ip;
    std::vector<ResourceRecordView> records;
};

std::string to_json(const DiscoveryEvent& ev);
