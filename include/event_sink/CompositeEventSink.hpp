#pragma once

#include "EventSink.hpp"
#include <vector>

std::shared_ptr<EventSink>
make_composite_event_sink(std::vector<EventSinkPtr> children);
