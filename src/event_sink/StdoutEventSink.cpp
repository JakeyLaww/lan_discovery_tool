#include "event_sink/StdoutEventSink.hpp"
#include "discovery/RecordLabels.hpp"
#include "mdns/DnsTypes.hpp"
#include <sstream>

namespace {

const char* type_name(uint16_t type) {
    switch (type) {
        case DnsType::A: return "A";
        case DnsType::PTR: return "PTR";
        case DnsType::SRV: return "SRV";
        case DnsType::TXT: return "TXT";
        default: return "RR";
    }
}

} // namespace

class StdoutEventSink : public EventSink {
public:
    explicit StdoutEventSink(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {}

    void on_event(const DiscoveryEvent& ev) override {
        const std::string ts = format_timestamp_iso(ev.timestamp_ms);
        for (const auto& rec : ev.records) {
            std::ostringstream ss;
            ss << "discovery"
               << " ts=" << ts
               << " src=" << ev.src_ip
               << " type=" << type_name(rec.type)
               << " owner=" << rec.owner_name
               << " rdata=" << rec.rdata_text;

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

std::shared_ptr<EventSink> make_stdout_event_sink(std::shared_ptr<Logger> logger) {
    return std::make_shared<StdoutEventSink>(std::move(logger));
}
