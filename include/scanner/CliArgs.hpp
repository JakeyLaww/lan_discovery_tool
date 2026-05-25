#pragma once

#include <string>

bool cli_has_flag(int argc, char *argv[], const char *flag);

std::string cli_get_option_value(int argc, char *argv[], const char *flag);
