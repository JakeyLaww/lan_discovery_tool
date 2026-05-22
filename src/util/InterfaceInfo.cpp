#include "util/InterfaceInfo.hpp"
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>

std::vector<InterfaceAddress> list_ipv4_interfaces() {
    std::vector<InterfaceAddress> out;
    struct ifaddrs* ifap = nullptr;
    if (getifaddrs(&ifap) != 0) {
        return out;
    }

    for (struct ifaddrs* ia = ifap; ia != nullptr; ia = ia->ifa_next) {
        if (!ia->ifa_addr || ia->ifa_addr->sa_family != AF_INET) continue;
        if ((ia->ifa_flags & IFF_LOOPBACK) != 0) continue;
        if ((ia->ifa_flags & IFF_UP) == 0) continue;

        char ipbuf[INET_ADDRSTRLEN];
        const auto* sin = reinterpret_cast<struct sockaddr_in*>(ia->ifa_addr);
        if (!inet_ntop(AF_INET, &sin->sin_addr, ipbuf, sizeof(ipbuf))) continue;

        out.push_back(InterfaceAddress{ia->ifa_name ? ia->ifa_name : "", ipbuf});
    }

    freeifaddrs(ifap);
    return out;
}

std::string ipv4_for_interface(const std::string& ifname) {
    for (const auto& entry : list_ipv4_interfaces()) {
        if (entry.name == ifname) return entry.ipv4;
    }
    return {};
}
