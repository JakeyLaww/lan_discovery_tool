#include "discovery/MdnsProbePlanner.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"
#include "util/KeyBuilder.hpp"
#include <string>

MdnsProbePlanner::MdnsProbePlanner(MdnsProbePlannerConfig config)
    : config_(config) {}

std::string MdnsProbePlanner::probe_key(const std::string &qname,
                                        uint16_t qtype) const {
  return join_key({qname, std::to_string(qtype)});
}

bool MdnsProbePlanner::try_enqueue(const std::string &qname, uint16_t qtype) {
  if (qname.empty())
    return false;

  const std::string key = probe_key(qname, qtype);
  const auto now = std::chrono::steady_clock::now();

  if (pending_keys_.count(key) != 0)
    return false;

  const auto last_it = last_probe_.find(key);
  if (last_it != last_probe_.end()) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_it->second);
    if (elapsed.count() < static_cast<int64_t>(config_.probe_cooldown_ms)) {
      return false;
    }
  }

  pending_keys_.insert(key);
  last_probe_[key] = now;
  return true;
}

void MdnsProbePlanner::enqueue_batch(MdnsProbeBatch batch) {
  if (batch.empty())
    return;
  if (pending_.size() >= config_.max_pending)
    return;

  MdnsProbeBatch accepted;
  accepted.reserve(batch.size());
  for (auto &req : batch) {
    if (try_enqueue(req.qname, req.qtype)) {
      accepted.push_back(std::move(req));
    }
  }

  if (!accepted.empty()) {
    pending_.push_back(std::move(accepted));
  }
}

void MdnsProbePlanner::observe(const DiscoveryEvent &ev) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto &rec : ev.records) {
    if (rec.type != DnsType::PTR)
      continue;
    if (rec.rdata_text.empty())
      continue;
    if (rec.rdata_text.find("(malformed") != std::string::npos)
      continue;

    if (rec.owner_name == MdnsNames::kDnsSdMetaService &&
        MdnsNames::is_service_type_name(rec.rdata_text)) {
      enqueue_batch(
          MdnsProbeBatch{MdnsProbeRequest{rec.rdata_text, DnsType::PTR}});
      continue;
    }

    if (MdnsNames::is_service_type_name(rec.owner_name) &&
        MdnsNames::is_instance_name(rec.rdata_text)) {
      MdnsProbeBatch batch;
      batch.push_back(MdnsProbeRequest{rec.rdata_text, DnsType::SRV});
      batch.push_back(MdnsProbeRequest{rec.rdata_text, DnsType::TXT});
      enqueue_batch(std::move(batch));
    }
  }
}

std::vector<MdnsProbeBatch> MdnsProbePlanner::drain_ready(size_t max_batches) {
  std::lock_guard<std::mutex> lock(mutex_);

  const size_t limit =
      max_batches > 0 ? max_batches : config_.max_probes_per_tick;
  std::vector<MdnsProbeBatch> out;
  out.reserve(limit);

  while (!pending_.empty() && out.size() < limit) {
    MdnsProbeBatch batch = std::move(pending_.front());
    pending_.pop_front();
    for (const auto &req : batch) {
      pending_keys_.erase(probe_key(req.qname, req.qtype));
    }
    out.push_back(std::move(batch));
  }

  return out;
}
