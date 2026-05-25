#include "core/DeviceStateStore.hpp"
#include "discovery/DiscoveryRecord.hpp"
#include "mdns/DnsTypes.hpp"
#include <cassert>
#include <iostream>

int main() {
  DeviceStateStore store;
  const std::string src = "192.168.1.10";

  ResourceRecordView rec_a{
      "Living Room._airplay._tcp.local", DnsType::SRV, "0 0 7000 tv.local",
      120, 1'700'000'000'000};
  ResourceRecordView rec_b{
      "Living Room._airplay._tcp.local", DnsType::TXT, "txtvers=1", 120,
      1'700'000'000'001};

  auto first = store.filter_unseen(src, {rec_a});
  assert(first.size() == 1);

  auto dup = store.filter_unseen(src, {rec_a});
  assert(dup.empty());

  auto second = store.filter_unseen(src, {rec_a, rec_b});
  assert(second.size() == 1);
  assert(second[0].type == DnsType::TXT);

  std::cout << "  ✓ device state store filter_unseen passed" << std::endl;
  std::cout << std::endl << "✓ All device state store tests passed!" << std::endl;
  return 0;
}
