#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct MdnsProbeRequest {
  std::string qname;
  uint16_t qtype;
};

using MdnsProbeBatch = std::vector<MdnsProbeRequest>;

struct MdnsProbePlannerConfig {
  size_t max_pending = 256;
  size_t max_probes_per_tick = 8;
  uint32_t probe_cooldown_ms = 60'000;
};

/**
 * @brief Schedules RFC 6763-style follow-up mDNS queries from observed records.
 *
 * Thread-safe: worker calls observe(); poller calls drain_ready() and sends.
 */
class MdnsProbePlanner {
public:
  explicit MdnsProbePlanner(MdnsProbePlannerConfig config = {});

  void observe(const DiscoveryEvent &ev);

  std::vector<MdnsProbeBatch> drain_ready(size_t max_batches);

private:
  std::string probe_key(const std::string &qname, uint16_t qtype) const;

  bool try_enqueue(const std::string &qname, uint16_t qtype);

  void enqueue_batch(MdnsProbeBatch batch);

  MdnsProbePlannerConfig config_;
  std::mutex mutex_;
  std::deque<MdnsProbeBatch> pending_;
  std::unordered_set<std::string> pending_keys_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      last_probe_;
};
