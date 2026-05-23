#include "discovery/DiscoveryTypes.hpp"
#include "mdns/DnsTypes.hpp"
#include <cassert>
#include <iostream>

void test_json_includes_ttl_and_last_seen() {
  DiscoveryEvent ev;
  ev.timestamp_ms = 1'700'000'000'000;
  ev.src_ip = "192.168.1.10";
  ev.records.push_back(ResourceRecordView{
      "test.local", DnsType::A, "192.168.1.1", 120, 1'700'000'000'123});

  const std::string json = to_json(ev);
  assert(json.find("\"ttl\":120") != std::string::npos);
  assert(json.find("\"last_seen_ms\":1700000000123") != std::string::npos);
  assert(json.find("192.168.1.10") != std::string::npos);

  std::cout << "  ✓ json includes ttl and last_seen_ms passed" << std::endl;
}

int main() {
  std::cout << "Running discovery JSON tests..." << std::endl << std::endl;
  try {
    test_json_includes_ttl_and_last_seen();
    std::cout << std::endl << "✓ All discovery JSON tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "✗ Discovery JSON test failed: " << ex.what() << std::endl;
    return 1;
  }
}
