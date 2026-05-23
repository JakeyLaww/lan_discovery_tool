#include "discovery/DiscoveryRecord.hpp"
#include "util/KeyBuilder.hpp"
#include <string>

ResourceRecordView make_record_view(const MdnsResourceRecord &rr,
                                    const uint8_t *message, size_t message_len,
                                    uint64_t observed_at_ms) {
  ResourceRecordView view;
  view.owner_name = rr.name;
  view.type = rr.type;
  view.rdata_text = rr.rdata_str(message, message_len);
  view.ttl = rr.ttl;
  view.observed_at_ms = observed_at_ms;
  return view;
}

std::string record_content_key(const ResourceRecordView &rec) {
  return join_key({rec.owner_name, std::to_string(rec.type), rec.rdata_text});
}

std::string record_store_key(const std::string &src_ip,
                             const ResourceRecordView &rec) {
  return join_key({src_ip, record_content_key(rec)});
}
