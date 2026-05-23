#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include "mdns/Parser.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

ResourceRecordView make_record_view(const MdnsResourceRecord &rr,
                                    const uint8_t *message, size_t message_len,
                                    uint64_t observed_at_ms);

std::string record_content_key(const ResourceRecordView &rec);

std::string record_store_key(const std::string &src_ip,
                             const ResourceRecordView &rec);
