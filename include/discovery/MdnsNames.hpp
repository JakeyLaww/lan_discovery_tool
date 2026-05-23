#pragma once

#include "discovery/DiscoveryTypes.hpp"
#include <string>

namespace MdnsNames {

constexpr const char* kDnsSdMetaService = "_services._dns-sd._udp.local";

bool ends_with(const std::string& s, const std::string& suffix);

bool has_mdns_service_suffix(const std::string& name);

/** e.g. _airplay._tcp.local (three labels) */
bool is_service_type_name(const std::string& name);

/** e.g. Living Room._airplay._tcp.local (four or more labels) */
bool is_instance_name(const std::string& name);

int label_count(const std::string& name);

/** Bonjour service/instance/host records useful for LAN audit output. */
bool is_audit_relevant_record(const ResourceRecordView& rec);

/** Extract _foo._tcp.local from a service type or instance name. */
std::string service_type_from_name(const std::string& name);

} // namespace MdnsNames
