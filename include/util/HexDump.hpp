#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

class Logger;

/**
 * @brief Utility for formatting data as hexadecimal dumps.
 *
 * Useful for debugging network protocols and binary data.
 */
class HexDump {
public:
    /**
     * @brief Print a hex dump of the provided byte vector.
     *
     * Format: 16 bytes per row, with offset, hex values, and optional ASCII.
     *
     * @param desc Short description to include in the dump header.
     * @param data Byte vector to display.
     * @param logger Logger instance to output the hex dump to.
     */
    static void print(const std::string& desc, const std::vector<uint8_t>& data,
                      const std::shared_ptr<Logger>& logger);

    /**
     * @brief Format data as a hex dump string.
     *
     * @param desc Short description to include in the dump header.
     * @param data Byte vector to format.
     * @return String representation of the hex dump.
     */
    static std::string format(const std::string& desc, const std::vector<uint8_t>& data);
};
