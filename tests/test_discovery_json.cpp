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
  assert(json.find("\"type_name\":\"A\"") != std::string::npos);

  std::cout << "  ✓ json includes ttl and last_seen_ms passed" << std::endl;
}

void test_json_srv_includes_service_and_host() {
  DiscoveryEvent ev;
  ev.timestamp_ms = 1'700'000'000'000;
  ev.src_ip = "192.168.1.10";
  ev.records.push_back(ResourceRecordView{
      "Living Room._airplay._tcp.local", DnsType::SRV,
      "priority=0 weight=0 port=7000 target=tv.local", 120, 1'700'000'000'123});

  const std::string json = to_json(ev);
  assert(json.find("\"type_name\":\"SRV\"") != std::string::npos);
  assert(json.find("\"service\":\"Living Room._airplay._tcp.local\"") !=
         std::string::npos);
  assert(json.find("\"host\":\"tv.local\"") != std::string::npos);

  std::cout << "  ✓ json SRV includes service and host passed" << std::endl;
}

int main() {
  std::cout << "Running discovery JSON tests..." << std::endl << std::endl;
  try {
    test_json_includes_ttl_and_last_seen();
    test_json_srv_includes_service_and_host();
    std::cout << std::endl << "✓ All discovery JSON tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "✗ Discovery JSON test failed: " << ex.what() << std::endl;
    return 1;
  }
}
