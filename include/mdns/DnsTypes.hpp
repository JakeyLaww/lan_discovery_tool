#pragma once

#include <cstdint>

/**
 * @brief DNS record type constants.
 *
 * Standard DNS record types used in DNS queries and responses.
 * Reference: RFC 1035
 */
namespace DnsType {
constexpr uint16_t A = 1;        // IPv4 host address
constexpr uint16_t NS = 2;       // Authoritative name server
constexpr uint16_t MD = 3;       // Mail destination (obsolete)
constexpr uint16_t MF = 4;       // Mail forwarder (obsolete)
constexpr uint16_t CNAME = 5;    // Canonical name for alias
constexpr uint16_t SOA = 6;      // Start of authority zone
constexpr uint16_t MB = 7;       // Mailbox domain name
constexpr uint16_t MG = 8;       // Mail group member
constexpr uint16_t MR = 9;       // Mail rename domain name
constexpr uint16_t NULL_RR = 10; // Null resource record
constexpr uint16_t WKS = 11;     // Well known service
constexpr uint16_t PTR = 12;     // Domain name pointer
constexpr uint16_t HINFO = 13;   // Host information
constexpr uint16_t MINFO = 14;   // Mailbox or mail list information
constexpr uint16_t MX = 15;      // Mail exchange
constexpr uint16_t TXT = 16;     // Text strings
constexpr uint16_t RP = 17;      // Responsible person
constexpr uint16_t AFSDB = 18;   // AFS database record
constexpr uint16_t X25 = 19;     // X.25 PSDN address
constexpr uint16_t ISDN = 20;    // ISDN address
constexpr uint16_t RT = 21;      // Route through
constexpr uint16_t NSAP = 22;    // NSAP address
constexpr uint16_t SRV = 33;     // Service record
constexpr uint16_t AAAA = 28;    // IPv6 host address
} // namespace DnsType

/**
 * @brief DNS class constants.
 */
namespace DnsClass {
constexpr uint16_t IN = 1; // Internet
constexpr uint16_t CS = 2; // CSNET class
constexpr uint16_t CH = 3; // Chaos class
constexpr uint16_t HS = 4; // Hesiod class
} // namespace DnsClass

/** Short name for discovery log output. */
const char *dns_type_name(uint16_t type);
