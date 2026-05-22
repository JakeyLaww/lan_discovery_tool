#pragma once

#include <string>
#include <vector>

struct InterfaceAddress {
    std::string name;
    std::string ipv4;
};

/** Lists non-loopback IPv4 addresses on local interfaces. */
std::vector<InterfaceAddress> list_ipv4_interfaces();

/**
 * Resolves an interface name (e.g. eth0) to its first IPv4 address.
 * @return empty string if not found.
 */
std::string ipv4_for_interface(const std::string& ifname);
