#include "discovery/MacResolver.hpp"
#include <cassert>
#include <iostream>

void test_lookup_in_table() {
  const char *table =
      "IP address       HW type     Flags       HW address            Mask     Device\n"
      "10.128.79.93     0x1         0x2         aa:bb:cc:dd:ee:ff     *        eth0\n"
      "10.128.79.99     0x1         0x2         00:00:00:00:00:00     *        eth0\n";

  const auto mac = MacResolver::lookup_in_table(table, "10.128.79.93");
  assert(mac.has_value());
  assert(*mac == "AA:BB:CC:DD:EE:FF");

  assert(!MacResolver::lookup_in_table(table, "10.128.79.99").has_value());
  assert(!MacResolver::lookup_in_table(table, "10.0.0.1").has_value());

  std::cout << "  ✓ lookup_in_table passed" << std::endl;
}

void test_normalize_mac_dash() {
  const auto mac = MacResolver::normalize_mac("aa-bb-cc-dd-ee-ff");
  assert(mac.has_value());
  assert(*mac == "AA:BB:CC:DD:EE:FF");
  std::cout << "  ✓ normalize_mac passed" << std::endl;
}

int main() {
  std::cout << "Running MacResolver tests..." << std::endl << std::endl;
  try {
    test_normalize_mac_dash();
    test_lookup_in_table();
    std::cout << std::endl << "✓ All MacResolver tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "✗ MacResolver test failed: " << ex.what() << std::endl;
    return 1;
  }
}
