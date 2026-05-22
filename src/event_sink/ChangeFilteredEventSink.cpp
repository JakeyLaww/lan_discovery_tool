#include "event_sink/ChangeFilteredEventSink.hpp"

class ChangeFilteredEventSink : public EventSink {
public:
    ChangeFilteredEventSink(std::shared_ptr<DeviceStateStore> store, EventSinkPtr inner)
        : store_(std::move(store)), inner_(std::move(inner)) {}

    void on_event(const DiscoveryEvent& ev) override {
        if (!store_ || !inner_) return;
        if (ev.records.empty()) return;
        if (!store_->update_and_changed(ev)) return;
        inner_->on_event(ev);
    }

private:
    std::shared_ptr<DeviceStateStore> store_;
    EventSinkPtr inner_;
};

std::shared_ptr<EventSink> make_change_filtered_event_sink(
    std::shared_ptr<DeviceStateStore> store,
    EventSinkPtr inner) {
    return std::make_shared<ChangeFilteredEventSink>(std::move(store), std::move(inner));
}
