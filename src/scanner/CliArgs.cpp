#include "scanner/CliArgs.hpp"
#include <cstring>

bool cli_has_flag(int argc, char *argv[], const char *flag) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], flag) == 0)
      return true;
  }
  return false;
}

std::string cli_get_option_value(int argc, char *argv[], const char *flag) {
  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], flag) == 0)
      return argv[i + 1];
  }
  return {};
}
