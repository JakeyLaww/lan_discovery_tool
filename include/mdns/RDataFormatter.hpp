#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Strategy interface for formatting DNS resource record data (RDATA).
 *
 * Different DNS record types (A, AAAA, PTR, SRV, TXT, etc.) have different
 * binary formats. Subclasses implement format-specific parsing and conversion
 * to human-readable strings.
 */
struct RDataFormatContext {
  const uint8_t *message = nullptr;
  size_t message_len = 0;
  size_t rdata_offset = 0;
  const std::vector<uint8_t> *rdata = nullptr;
};

class RDataFormatter {
public:
  virtual ~RDataFormatter() = default;
  virtual std::string format(const RDataFormatContext &ctx) const = 0;
};

using RDataFormatterPtr = std::shared_ptr<RDataFormatter>;

/**
 * @brief Format IPv4 address (A record).
 * RDATA: 4 bytes representing 4 octets in network byte order.
 */
class ARecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Format IPv6 address (AAAA record).
 * RDATA: 16 bytes representing 8 16-bit groups in network byte order.
 */
class AAAARecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Format domain name pointer (PTR record).
 * RDATA: A compressed domain name (QNAME) format.
 */
class PTRRecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Format service record (SRV record).
 * RDATA: Priority (2), Weight (2), Port (2), Target name (QNAME).
 */
class SRVRecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Format text record (TXT record).
 * RDATA: Series of length-prefixed character strings.
 */
class TXTRecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Generic formatter for unknown record types.
 * Falls back to hex dump representation.
 */
class GenericRecordFormatter : public RDataFormatter {
public:
  std::string format(const RDataFormatContext &ctx) const override;
};

/**
 * @brief Factory function to get the appropriate formatter for a DNS record
 * type.
 *
 * @param type DNS record type code (1=A, 28=AAAA, 12=PTR, 33=SRV, 16=TXT, etc.)
 * @return Formatter instance for the given type, or GenericRecordFormatter if
 * unknown.
 */
RDataFormatterPtr get_rdata_formatter(uint16_t type);
