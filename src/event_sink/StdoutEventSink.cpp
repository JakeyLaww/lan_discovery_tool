#include "event_sink/StdoutEventSink.hpp"
#include "discovery/RecordLabels.hpp"
#include "mdns/DnsTypes.hpp"
#include <sstream>

class StdoutEventSink : public EventSink {
public:
  explicit StdoutEventSink(std::shared_ptr<Logger> logger)
      : logger(std::move(logger)) {}

  void on_event(const DiscoveryEvent &ev) override {
    for (const auto &rec : ev.records) {
      const uint64_t seen_ms =
          rec.observed_at_ms != 0 ? rec.observed_at_ms : ev.timestamp_ms;
      std::ostringstream ss;
      ss << "discovery" << " ts=" << format_timestamp_iso(seen_ms)
         << " src=" << ev.src_ip << " type=" << dns_type_name(rec.type)
         << " owner=" << rec.owner_name << " | rdata=" << rec.rdata_text
         << " ttl=" << rec.ttl
         << " last_seen=" << format_timestamp_iso(seen_ms);

      const std::string service = service_label(rec);
      if (!service.empty()) {
        ss << " service=" << service;
      }
      const std::string host = host_label(rec);
      if (!host.empty()) {
        ss << " host=" << host;
      }

      logger->info(ss.str());
    }
  }

private:
  std::shared_ptr<Logger> logger;
};

std::shared_ptr<EventSink>
make_stdout_event_sink(std::shared_ptr<Logger> logger) {
  return std::make_shared<StdoutEventSink>(std::move(logger));
}
