#pragma once

#include "EventSink.hpp"
#include "core/DeviceStateStore.hpp"
#include <memory>

std::shared_ptr<EventSink>
make_change_filtered_event_sink(std::shared_ptr<DeviceStateStore> store,
                                EventSinkPtr inner);
