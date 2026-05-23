#include "discovery/DiscoveryTypes.hpp"
#include "discovery/MdnsProbePlanner.hpp"
#include "mdns/DnsTypes.hpp"
#include <cassert>
#include <iostream>

static DiscoveryEvent make_ptr_event(const std::string &owner,
                                     const std::string &rdata) {
  DiscoveryEvent ev;
  ev.timestamp_ms = 1;
  ev.src_ip = "192.168.1.10";
  ev.records.push_back(ResourceRecordView{owner, DnsType::PTR, rdata});
  return ev;
}

void test_meta_ptr_enqueues_service_type_ptr() {
  MdnsProbePlanner planner;
  planner.observe(
      make_ptr_event("_services._dns-sd._udp.local", "_airplay._tcp.local"));

  auto batches = planner.drain_ready(8);
  assert(batches.size() == 1);
  assert(batches[0].size() == 1);
  assert(batches[0][0].qname == "_airplay._tcp.local");
  assert(batches[0][0].qtype == DnsType::PTR);

  std::cout << "  ✓ meta PTR enqueues service-type PTR passed" << std::endl;
}

void test_service_type_ptr_enqueues_srv_txt() {
  MdnsProbePlanner planner;
  planner.observe(
      make_ptr_event("_airplay._tcp.local", "Living Room._airplay._tcp.local"));

  auto batches = planner.drain_ready(8);
  assert(batches.size() == 1);
  assert(batches[0].size() == 2);
  assert(batches[0][0].qname == "Living Room._airplay._tcp.local");
  assert(batches[0][1].qname == "Living Room._airplay._tcp.local");
  assert(batches[0][0].qtype == DnsType::SRV);
  assert(batches[0][1].qtype == DnsType::TXT);

  std::cout << "  ✓ service-type PTR enqueues SRV+TXT passed" << std::endl;
}

void test_dedup_same_target_not_queued_twice() {
  MdnsProbePlanner planner;
  const auto ev =
      make_ptr_event("_services._dns-sd._udp.local", "_airplay._tcp.local");
  planner.observe(ev);
  planner.drain_ready(8);
  planner.observe(ev);

  auto batches = planner.drain_ready(8);
  assert(batches.empty());

  std::cout << "  ✓ dedup same target passed" << std::endl;
}

void test_cooldown_blocks_requeue() {
  MdnsProbePlannerConfig config;
  config.probe_cooldown_ms = 60'000;
  MdnsProbePlanner planner(config);

  const auto ev =
      make_ptr_event("_services._dns-sd._udp.local", "_http._tcp.local");
  planner.observe(ev);
  planner.drain_ready(8);
  planner.observe(ev);

  auto batches = planner.drain_ready(8);
  assert(batches.empty());

  std::cout << "  ✓ cooldown blocks requeue passed" << std::endl;
}

int main() {
  std::cout << "Running probe planner tests..." << std::endl << std::endl;
  try {
    test_meta_ptr_enqueues_service_type_ptr();
    test_service_type_ptr_enqueues_srv_txt();
    test_dedup_same_target_not_queued_twice();
    test_cooldown_blocks_requeue();
    std::cout << std::endl << "✓ All probe planner tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "✗ Probe planner test failed: " << ex.what() << std::endl;
    return 1;
  }
}
