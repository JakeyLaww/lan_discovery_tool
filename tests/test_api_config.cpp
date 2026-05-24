#include "scanner/ApiConfig.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>

int main() {
  const char *argv[] = {"scanner", "--api-url", "http://example.com/",
                        "--api-token", "tok"};
  unsetenv("LAN_API_URL");
  unsetenv("LAN_API_TOKEN");

  const ApiConfig config = load_api_config(5, const_cast<char **>(argv));
  assert(config.base_url == "http://example.com");
  assert(config.api_token == "tok");
  assert(config.has_url());

  std::cout << "  ✓ api config CLI parsing passed" << std::endl;
  std::cout << std::endl << "✓ All api config tests passed!" << std::endl;
  return 0;
}
