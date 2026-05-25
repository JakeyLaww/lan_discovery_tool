#include "scanner/ApiConfig.hpp"
#include "scanner/CliArgs.hpp"
#include <cstdlib>
#include <string>

namespace {

std::string trim_trailing_slashes(std::string url) {
  while (!url.empty() && url.back() == '/')
    url.pop_back();
  return url;
}

std::string getenv_string(const char *name) {
  const char *value = std::getenv(name);
  return value ? std::string(value) : std::string{};
}

} // namespace

ApiConfig load_api_config(int argc, char *argv[]) {
  ApiConfig config;
  config.base_url = trim_trailing_slashes(getenv_string("LAN_API_URL"));
  config.api_token = getenv_string("LAN_API_TOKEN");

  const std::string cli_url = cli_get_option_value(argc, argv, "--api-url");
  if (!cli_url.empty())
    config.base_url = trim_trailing_slashes(cli_url);

  const std::string cli_token = cli_get_option_value(argc, argv, "--api-token");
  if (!cli_token.empty())
    config.api_token = cli_token;

  return config;
}

std::string discovery_events_post_url(const ApiConfig &api) {
  return api.base_url + "/v1/discovery/events";
}
