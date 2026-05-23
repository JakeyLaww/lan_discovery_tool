#pragma once

#include "EventSink.hpp"
#include "Logger.hpp"
#include "discovery/DeviceRegistry.hpp"
#include <memory>

std::shared_ptr<EventSink> make_device_summary_event_sink(
    std::shared_ptr<DeviceRegistry> registry,
    std::shared_ptr<Logger> logger,
    EventSinkPtr inner);
