#include "util/InterfaceInfo.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

std::vector<InterfaceAddress> list_ipv4_interfaces() {
  std::vector<InterfaceAddress> out;
  struct ifaddrs *ifap = nullptr;
  if (getifaddrs(&ifap) != 0) {
    return out;
  }

  for (struct ifaddrs *ia = ifap; ia != nullptr; ia = ia->ifa_next) {
    if (!ia->ifa_addr || ia->ifa_addr->sa_family != AF_INET)
      continue;
    if ((ia->ifa_flags & IFF_LOOPBACK) != 0)
      continue;
    if ((ia->ifa_flags & IFF_UP) == 0)
      continue;

    char ipbuf[INET_ADDRSTRLEN];
    const auto *sin = reinterpret_cast<struct sockaddr_in *>(ia->ifa_addr);
    if (!inet_ntop(AF_INET, &sin->sin_addr, ipbuf, sizeof(ipbuf)))
      continue;

    out.push_back(InterfaceAddress{ia->ifa_name ? ia->ifa_name : "", ipbuf});
  }

  freeifaddrs(ifap);
  return out;
}

std::string ipv4_for_interface(const std::string &ifname) {
  for (const auto &entry : list_ipv4_interfaces()) {
    if (entry.name == ifname)
      return entry.ipv4;
  }
  return {};
}

namespace {

int multicast_interface_priority(const std::string &ipv4) {
  if (ipv4.rfind("169.254.", 0) == 0)
    return -1;
  if (ipv4.rfind("10.", 0) == 0)
    return 3;
  if (ipv4.rfind("192.168.", 0) == 0)
    return 2;
  if (ipv4.rfind("172.", 0) == 0)
    return 1;
  return 0;
}

} // namespace

std::optional<InterfaceAddress> pick_default_multicast_interface() {
  const auto ifaces = list_ipv4_interfaces();
  const InterfaceAddress *best = nullptr;
  int best_pri = -1;
  for (const auto &iface : ifaces) {
    const int pri = multicast_interface_priority(iface.ipv4);
    if (pri > best_pri) {
      best_pri = pri;
      best = &iface;
    }
  }
  if (best)
    return *best;
  if (!ifaces.empty())
    return ifaces.front();
  return std::nullopt;
}
