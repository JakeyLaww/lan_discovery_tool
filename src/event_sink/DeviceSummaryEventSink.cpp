#include "event_sink/DeviceSummaryEventSink.hpp"

class DeviceSummaryEventSink : public EventSink {
public:
  DeviceSummaryEventSink(std::shared_ptr<DeviceRegistry> registry,
                         std::shared_ptr<Logger> logger, EventSinkPtr inner)
      : registry_(std::move(registry)), logger_(std::move(logger)),
        inner_(std::move(inner)) {}

  void on_event(const DiscoveryEvent &ev) override {
    if (registry_ && logger_) {
      if (registry_->apply(ev)) {
        const std::string summary = registry_->format_summary(ev.src_ip);
        if (!summary.empty()) {
          logger_->info(summary);
        }
      }
    }
    if (inner_)
      inner_->on_event(ev);
  }

private:
  std::shared_ptr<DeviceRegistry> registry_;
  std::shared_ptr<Logger> logger_;
  EventSinkPtr inner_;
};

std::shared_ptr<EventSink>
make_device_summary_event_sink(std::shared_ptr<DeviceRegistry> registry,
                               std::shared_ptr<Logger> logger,
                               EventSinkPtr inner) {
  return std::make_shared<DeviceSummaryEventSink>(
      std::move(registry), std::move(logger), std::move(inner));
}
