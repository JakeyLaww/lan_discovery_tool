#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace MacResolver {

std::optional<std::string> normalize_mac(std::string mac);

/** Parse ARP table text (e.g. /proc/net/arp contents) for an IPv4 address. */
std::optional<std::string> lookup_in_table(std::string_view arp_table,
                                           const std::string &src_ip);

/** Linux /proc/net/arp lookup for IPv4; returns AA:BB:CC:DD:EE:FF or nullopt. */
std::optional<std::string> lookup(const std::string &src_ip);

} // namespace MacResolver
