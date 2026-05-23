#pragma once

#include <cstdint>
#include <set>
#include <string>

struct DeviceSnapshot {
    std::string src_ip;
    uint64_t last_seen_ms = 0;
    std::string mdns_host;
    std::set<std::string> service_types;
    std::set<std::string> instances;
};
