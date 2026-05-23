#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <cstdint>
#include <string>

std::string format_timestamp_iso(uint64_t timestamp_ms);

std::string service_label(const ResourceRecordView &rec);

std::string host_label(const ResourceRecordView &rec);
