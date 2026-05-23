#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <memory>

class EventSink {
public:
  virtual ~EventSink() = default;
  virtual void on_event(const DiscoveryEvent &ev) = 0;
};

using EventSinkPtr = std::shared_ptr<EventSink>;
