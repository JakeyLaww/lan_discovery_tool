#pragma once

#include "discovery/DiscoveryTypes.hpp"

/** True when a record should appear in discovery output (type + Bonjour
 * semantics). */
bool is_displayable_record(const ResourceRecordView &rec);
