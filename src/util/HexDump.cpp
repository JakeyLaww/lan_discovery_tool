#include "util/HexDump.hpp"
#include "Logger.hpp"
#include <iomanip>
#include <sstream>

std::string HexDump::format(const std::string &desc,
                            const std::vector<uint8_t> &data) {
  std::ostringstream oss;
  oss << "--- " << desc << " (" << std::dec << data.size() << " bytes) ---\n";
  for (size_t i = 0; i < data.size(); i++) {
    if (i % 16 == 0) {
      oss << std::hex << std::setw(4) << std::setfill('0') << i << "  ";
    }
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(data[i]) << " ";
    if (i % 16 == 15 || i == data.size() - 1) {
      oss << "\n";
    }
  }
  oss << "\n";
  return oss.str();
}

void HexDump::print(const std::string &desc, const std::vector<uint8_t> &data,
                    const std::shared_ptr<Logger> &logger) {
  if (!logger)
    return;
  logger->debug(format(desc, data));
}
