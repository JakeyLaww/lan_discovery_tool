#pragma once

#include <chrono>
#include <fstream>
#include <string>

// #region agent log
inline void agent_debug_log(const char* hypothesis_id,
                            const char* location,
                            const char* message,
                            const std::string& data_json) {
    std::ofstream out("/home/jake/myrepos/lan_discovery_tool/.cursor/debug-216817.log",
                      std::ios::app);
    if (!out) return;
    const auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    out << "{\"sessionId\":\"216817\",\"hypothesisId\":\"" << hypothesis_id
        << "\",\"location\":\"" << location << "\",\"message\":\"" << message
        << "\",\"data\":" << data_json << ",\"timestamp\":" << ts << "}\n";
}
// #endregion
