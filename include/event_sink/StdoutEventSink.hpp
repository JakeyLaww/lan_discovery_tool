#pragma once

#include "EventSink.hpp"
#include "Logger.hpp"
#include <memory>

std::shared_ptr<EventSink>
make_stdout_event_sink(std::shared_ptr<Logger> logger);
