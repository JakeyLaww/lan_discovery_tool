#include "discovery/RecordFilter.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"

namespace {

bool is_interesting_record_type(uint16_t type) {
    switch (type) {
        case DnsType::A:
        case DnsType::PTR:
        case DnsType::SRV:
        case DnsType::TXT:
            return true;
        default:
            return false;
    }
}

} // namespace

bool is_displayable_record(const ResourceRecordView& rec) {
    return is_interesting_record_type(rec.type) && MdnsNames::is_audit_relevant_record(rec);
}
