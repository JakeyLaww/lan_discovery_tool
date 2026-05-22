#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ResourceRecordView {
    std::string owner_name;
    uint16_t type;
    std::string rdata_text;
};

struct DiscoveryEvent {
    uint64_t timestamp_ms;
    std::string src_ip;
    std::vector<ResourceRecordView> records;
};

std::string json_escape(const std::string& s);

std::string to_json(const DiscoveryEvent& ev);
