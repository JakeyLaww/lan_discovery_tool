#pragma once

#include <string>

struct ApiConfig {
  std::string base_url;
  std::string api_token;

  bool has_url() const { return !base_url.empty(); }
};

ApiConfig load_api_config(int argc, char *argv[]);

/** POST target for discovery events (base URL without trailing slash). */
std::string discovery_events_post_url(const ApiConfig &api);
