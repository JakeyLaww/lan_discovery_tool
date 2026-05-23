#pragma once
#include "mdns/DnsProtocol.hpp"
#include "util/BinaryStreamReader.hpp"
#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>
#include <stdexcept>
#include <arpa/inet.h>

namespace MDNS {
    constexpr uint16_t PORT = 5353;
    constexpr const char* MULTICAST_GROUP = "224.0.0.251";

    /**
     * @brief DNS header with host-order fields.
     */
    struct DnsHeaderHost {
        uint16_t transaction_id;
        uint16_t flags;
        uint16_t questions;
        uint16_t answer_rrs;
        uint16_t authority_rrs;
        uint16_t additional_rrs;
    };

    inline uint16_t read_u16_be(const uint8_t* p) {
        return BinaryStreamReader::read_u16_be(p);
    }

    inline uint32_t read_u32_be(const uint8_t* p) {
        return BinaryStreamReader::read_u32_be(p);
    }

    /**
     * @brief Parse the DNS header from a network buffer into host-order fields.
     *
     * This implementation explicitly decodes big-endian fields and avoids
     * aliasing/ alignment issues that can occur with reinterpret_cast.
     *
     * @param buf Pointer to the buffer containing the DNS message.
     * @param len Length of the buffer in bytes.
     * @return DnsHeaderHost Parsed header with host-order numeric fields.
     */
    inline DnsHeaderHost parse_dns_header(const uint8_t* buf, size_t len) {
        if (len < DnsProtocol::kHeaderSize) throw std::invalid_argument("DNS buffer too small for header");
        DnsHeaderHost h;
        h.transaction_id = read_u16_be(buf + 0);
        h.flags = read_u16_be(buf + 2);
        h.questions = read_u16_be(buf + 4);
        h.answer_rrs = read_u16_be(buf + 6);
        h.authority_rrs = read_u16_be(buf + 8);
        h.additional_rrs = read_u16_be(buf + 10);
        return h;
    }
    // Parse a DNS QNAME at buffer[offset]. Returns the dotted name and advances offset
    /**
     * @brief Parse a domain name (QNAME) from a DNS message.
     *
     * Decodes QNAMEs including RFC 1035 compression pointers.
     *
     * @param buf Pointer to the DNS message buffer.
     * @param len Length of the buffer in bytes.
     * @param offset Starting offset within the buffer for the QNAME.
     * @return std::pair<std::string,size_t> The decoded name and new offset.
     */
    inline std::pair<std::string, size_t> parse_qname(const uint8_t* buf, size_t len, size_t offset) {
        std::string name;
        size_t pos = offset;
        size_t consumed = offset;
        bool jumped = false;
        size_t loop_count = 0;

        while (pos < len) {
            if (++loop_count > len) {
                throw std::invalid_argument("QNAME pointer loop detected");
            }

            uint8_t label_len = buf[pos];
            if (label_len == DnsProtocol::kQnameTerminator) {
                if (!jumped) {
                    consumed = pos + 1;
                }
                break;
            }

            if ((label_len & DnsProtocol::kQnamePointerPrefix) == DnsProtocol::kQnamePointerPrefix) {
                if (pos + 1 >= len) {
                    throw std::invalid_argument("Compressed QNAME pointer truncated");
                }
                uint16_t pointer = ((static_cast<uint16_t>(label_len & DnsProtocol::kQnamePointerOffsetMask) << 8) | buf[pos + 1]);
                if (pointer >= len) {
                    throw std::invalid_argument("Compressed QNAME pointer out of bounds");
                }
                if (!jumped) {
                    consumed = pos + 2;
                }
                pos = pointer;
                jumped = true;
                continue;
            }

            if (label_len & DnsProtocol::kQnamePointerPrefix) {
                throw std::invalid_argument("Invalid QNAME label length");
            }
            if (pos + 1 + label_len > len) {
                throw std::invalid_argument("QNAME runs past buffer");
            }
            if (!name.empty()) {
                name.push_back('.');
            }
            name.append(reinterpret_cast<const char*>(buf + pos + 1), label_len);
            pos += 1 + label_len;
            if (!jumped) {
                consumed = pos;
            }
        }

        if (pos >= len) {
            throw std::invalid_argument("QNAME not terminated before end of buffer");
        }
        return {name, consumed};
    }
}
