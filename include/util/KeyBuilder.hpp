#pragma once

#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

/** Joins parts with '|' for composite map/set keys. */
inline std::string join_key(std::initializer_list<std::string_view> parts) {
    std::ostringstream oss;
    bool first = true;
    for (const auto part : parts) {
        if (!first) {
            oss << '|';
        }
        oss << part;
        first = false;
    }
    return oss.str();
}
