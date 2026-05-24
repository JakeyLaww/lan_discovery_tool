#pragma once

#include <string>

struct ApiConfig {
  std::string base_url;
  std::string api_token;

  bool has_url() const { return !base_url.empty(); }
};

ApiConfig load_api_config(int argc, char *argv[]);
