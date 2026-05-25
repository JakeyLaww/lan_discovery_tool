#include "discovery/MacResolver.hpp"
#include <cctype>
#include <fstream>
#include <sstream>

namespace MacResolver {
namespace {

std::optional<std::string> parse_arp_line(const std::string &line,
                                          const std::string &src_ip) {
  std::istringstream iss(line);
  std::string ip, hw_type, flags, mac;
  if (!(iss >> ip >> hw_type >> flags >> mac))
    return std::nullopt;
  if (ip != src_ip)
    return std::nullopt;
  return normalize_mac(mac);
}

} // namespace

std::optional<std::string> normalize_mac(std::string mac) {
  if (mac == "00:00:00:00:00:00" || mac == "<incomplete>")
    return std::nullopt;
  for (char &c : mac) {
    if (c == '-')
      c = ':';
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  if (mac.size() != 17)
    return std::nullopt;
  for (size_t i = 0; i < mac.size(); ++i) {
    if (i % 3 == 2) {
      if (mac[i] != ':')
        return std::nullopt;
    } else if (!std::isxdigit(static_cast<unsigned char>(mac[i]))) {
      return std::nullopt;
    }
  }
  return mac;
}

std::optional<std::string> lookup_in_table(std::string_view arp_table,
                                           const std::string &src_ip) {
  std::istringstream stream{std::string(arp_table)};
  std::string line;
  std::getline(stream, line);
  while (std::getline(stream, line)) {
    if (auto mac = parse_arp_line(line, src_ip))
      return mac;
  }
  return std::nullopt;
}

std::optional<std::string> lookup(const std::string &src_ip) {
  std::ifstream arp("/proc/net/arp");
  if (!arp.is_open())
    return std::nullopt;
  std::ostringstream buffer;
  buffer << arp.rdbuf();
  return lookup_in_table(buffer.str(), src_ip);
}

} // namespace MacResolver
