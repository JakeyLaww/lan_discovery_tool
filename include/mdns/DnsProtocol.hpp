#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief DNS wire-format protocol constants (RFC 1035).
 *
 * Values are host-endian where they match parsed MdnsHeaderInfo fields.
 */
namespace DnsProtocol {
constexpr size_t kHeaderSize = 12;

// Header flags (host-endian)
constexpr uint16_t kFlagsQuery = 0x0000;
constexpr uint16_t kFlagsResponse = 0x8000;      // QR: query/response
constexpr uint16_t kFlagsAuthoritative = 0x0400; // AA: authoritative answer
constexpr uint16_t kFlagsMdnsResponse =
    0x8400; // QR | AA (typical mDNS response)

// QNAME encoding (RFC 1035 §4.1.4)
constexpr uint8_t kQnamePointerPrefix = 0xC0;
constexpr uint8_t kQnamePointerOffsetMask = 0x3F;
constexpr uint8_t kQnameTerminator = 0x00;
} // namespace DnsProtocol
