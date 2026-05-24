#pragma once

#include "EventSink.hpp"
#include "Logger.hpp"
#include "scanner/ApiConfig.hpp"
#include <cstddef>
#include <memory>

struct HttpEventSinkConfig {
  std::string post_url;
  std::string api_token;
  size_t max_queue_size = 256;
  long http_timeout_sec = 10;
};

HttpEventSinkConfig make_http_sink_config(const ApiConfig &api);

std::shared_ptr<EventSink> make_http_event_sink(HttpEventSinkConfig config,
                                                std::shared_ptr<Logger> logger);
