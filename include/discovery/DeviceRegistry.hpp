#pragma once

#include "discovery/DeviceSnapshot.hpp"
#include "discovery/DiscoveryTypes.hpp"
#include <string>
#include <unordered_map>

class DeviceRegistry {
public:
  static constexpr size_t kMaxInstancesInSummary = 5;

  bool apply(const DiscoveryEvent &ev);

  std::string format_summary(const std::string &src_ip) const;

  const DeviceSnapshot *find(const std::string &src_ip) const;

private:
  bool merge_records(DeviceSnapshot &snap, const DiscoveryEvent &ev);

  std::unordered_map<std::string, DeviceSnapshot> devices_;
};
