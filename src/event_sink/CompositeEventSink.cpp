#include "event_sink/CompositeEventSink.hpp"

class CompositeEventSink : public EventSink {
public:
  explicit CompositeEventSink(std::vector<EventSinkPtr> children)
      : children_(std::move(children)) {}

  void on_event(const DiscoveryEvent &ev) override {
    for (const auto &child : children_) {
      if (child)
        child->on_event(ev);
    }
  }

private:
  std::vector<EventSinkPtr> children_;
};

std::shared_ptr<EventSink>
make_composite_event_sink(std::vector<EventSinkPtr> children) {
  return std::make_shared<CompositeEventSink>(std::move(children));
}
