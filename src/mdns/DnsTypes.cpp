#include "mdns/DnsTypes.hpp"

const char *dns_type_name(uint16_t type) {
  switch (type) {
  case DnsType::A:
    return "A";
  case DnsType::PTR:
    return "PTR";
  case DnsType::SRV:
    return "SRV";
  case DnsType::TXT:
    return "TXT";
  default:
    return "RR";
  }
}
