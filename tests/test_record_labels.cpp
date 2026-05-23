#include "discovery/MdnsNames.hpp"
#include "discovery/RecordLabels.hpp"
#include "mdns/DnsTypes.hpp"
#include <cassert>
#include <iostream>

void test_srv_service_label_uses_instance() {
  const ResourceRecordView rec{"Living Room._airplay._tcp.local", DnsType::SRV,
                               "priority=0 weight=0 port=7000 target=tv.local",
                               120, 1000};

  assert(service_label(rec) == "Living Room._airplay._tcp.local");
  assert(host_label(rec) == "tv.local");
  std::cout << "  ✓ SRV service label uses instance passed" << std::endl;
}

void test_txt_instance_has_no_host_label() {
  const ResourceRecordView rec{"Living Room._airplay._tcp.local", DnsType::TXT,
                               "model=MacBookPro17,1", 120, 1000};

  assert(service_label(rec) == "Living Room._airplay._tcp.local");
  assert(host_label(rec).empty());
  std::cout << "  ✓ TXT instance has no host label passed" << std::endl;
}

void test_txt_plain_local_host_label() {
  const ResourceRecordView rec{"tv.local", DnsType::TXT, "txtvers=1", 120,
                               1000};

  assert(host_label(rec) == "tv.local");
  std::cout << "  ✓ TXT plain .local host label passed" << std::endl;
}

int main() {
  std::cout << "Running record labels tests..." << std::endl << std::endl;
  try {
    test_srv_service_label_uses_instance();
    test_txt_instance_has_no_host_label();
    test_txt_plain_local_host_label();
    std::cout << std::endl << "✓ All record labels tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "✗ Record labels test failed: " << ex.what() << std::endl;
    return 1;
  }
}
