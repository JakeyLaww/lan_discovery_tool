#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Utility for reading multi-byte values in big-endian (network byte
 * order) from binary streams.
 *
 * This namespace provides simple, type-safe helpers to read 16-bit and 32-bit
 * integers from unaligned network buffers without triggering alignment warnings
 * or aliasing issues. All functions read in big-endian (network) byte order, as
 * per DNS/mDNS specifications.
 */
namespace BinaryStreamReader {

/**
 * @brief Read a 16-bit unsigned integer in big-endian byte order.
 *
 * Reads two bytes at ptr[0] and ptr[1] and combines them as:
 *   (ptr[0] << 8) | ptr[1]
 *
 * @param ptr Pointer to buffer position (must point to at least 2 bytes).
 * @return uint16_t The value in host byte order.
 */
inline uint16_t read_u16_be(const uint8_t *ptr) {
  return (static_cast<uint16_t>(ptr[0]) << 8) | static_cast<uint16_t>(ptr[1]);
}

/**
 * @brief Read a 32-bit unsigned integer in big-endian byte order.
 *
 * Reads four bytes and combines them as:
 *   (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]
 *
 * @param ptr Pointer to buffer position (must point to at least 4 bytes).
 * @return uint32_t The value in host byte order.
 */
inline uint32_t read_u32_be(const uint8_t *ptr) {
  return (static_cast<uint32_t>(ptr[0]) << 24) |
         (static_cast<uint32_t>(ptr[1]) << 16) |
         (static_cast<uint32_t>(ptr[2]) << 8) | static_cast<uint32_t>(ptr[3]);
}

} // namespace BinaryStreamReader
