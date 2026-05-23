#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include "mdns/Parser.hpp"  // For MdnsParsedMessage, MdnsResourceRecord, etc.

/**
 * @brief Class-based DNS message decoder for mDNS wire format parsing.
 * 
 * MdnsMessageDecoder encapsulates the logic to parse a complete DNS message from
 * its wire format (binary buffer) into structured data types. It consolidates repeated
 * parsing patterns and eliminates code duplication in the procedural Parser.cpp.
 * 
 * This class is intended to coexist with the procedural Parser.cpp functions; the
 * procedural functions can delegate to this class for backward compatibility while
 * allowing new code to use the cleaner class-based API.
 * 
 * Example:
 *   MdnsMessageDecoder decoder(buffer.data(), buffer.size());
 *   try {
 *       auto msg = decoder.decode();
 *       // Use msg.answers, msg.questions, etc.
 *   } catch (const std::invalid_argument& ex) {
 *       logger->error(ex.what());
 *   }
 */
class MdnsMessageDecoder {
private:
    const uint8_t* buffer;
    size_t buffer_len;

    /**
     * @brief Parse a section of resource records (answers, authorities, or additionals).
     * 
     * @param count    Number of resource records to parse in this section.
     * @param pos      Current buffer position; updated to point after the last parsed record.
     * @return std::vector<MdnsResourceRecord> Parsed resource records.
     */
    std::vector<MdnsResourceRecord> parse_record_section(uint16_t count, size_t& pos);

    /**
     * @brief Parse a single resource record starting at the given position.
     * 
     * @param pos  Current buffer position; updated to point after the parsed record.
     * @return MdnsResourceRecord The parsed resource record.
     */
    MdnsResourceRecord parse_record_impl(size_t& pos);

public:
    /**
     * @brief Construct a decoder for the given buffer.
     * 
     * @param buf     Pointer to buffer containing wire format DNS message. Must not be nullptr.
     * @param len     Length of the buffer in bytes.
     */
    explicit MdnsMessageDecoder(const uint8_t* buf, size_t len);

    /**
     * @brief Parse the complete DNS message from wire format.
     * 
     * Decodes the DNS header, questions, answer records, authority records, and
     * additional records. Throws on malformed input.
     * 
     * @return MdnsParsedMessage Complete parsed message structure.
     * @throws std::invalid_argument if the buffer is malformed or truncated.
     */
    MdnsParsedMessage decode();
};
