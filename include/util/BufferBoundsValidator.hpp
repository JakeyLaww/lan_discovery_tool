#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

/**
 * @brief Utility for validating buffer access bounds with contextual error
 * messages.
 *
 * This namespace centralizes buffer bounds checking to replace scattered inline
 * checks throughout the codebase. Instead of repeated manual bounds validation,
 * callers invoke a single function that provides clear error context if bounds
 * are exceeded.
 */
namespace BufferBoundsValidator {

/**
 * @brief Validate that a buffer has enough bytes remaining at a given position.
 *
 * Throws std::invalid_argument if the buffer bounds would be exceeded.
 *
 * Example:
 *   BufferBoundsValidator::ensure_readable(pos, 2, buffer_len, "TYPE field");
 *   uint16_t type = BinaryStreamReader::read_u16_be(buf + pos);
 *   pos += 2;
 *
 * @param current_pos    Current position in buffer (0-based).
 * @param bytes_needed   Number of bytes to read from current_pos.
 * @param buffer_len     Total length of buffer.
 * @param field_name     Name of field being read (for error message context).
 *
 * @throws std::invalid_argument if current_pos + bytes_needed > buffer_len.
 *         Error message includes field_name for diagnostic clarity.
 */
void ensure_readable(size_t current_pos, size_t bytes_needed, size_t buffer_len,
                     const std::string &field_name);

} // namespace BufferBoundsValidator
