#pragma once

#include "MDNSDefinitions.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct MdnsHeaderInfo {
  uint16_t transaction_id;
  uint16_t flags;
  uint16_t questions;
  uint16_t answer_rrs;
  uint16_t authority_rrs;
  uint16_t additional_rrs;
};

struct MdnsQuestion {
  std::string name;
  uint16_t type;
  uint16_t class_code;
};

struct MdnsResourceRecord {
  std::string name;
  uint16_t type;
  uint16_t class_code;
  uint32_t ttl;
  std::vector<uint8_t> rdata;
  size_t rdata_offset = 0;

  // Interpret RDATA; name-bearing types need the full wire message for
  // compression.
  std::string rdata_str(const uint8_t *message, size_t message_len) const;
};

struct MdnsParsedMessage {
  MdnsHeaderInfo header;
  std::vector<MdnsQuestion> questions;
  std::vector<MdnsResourceRecord> answers;
  std::vector<MdnsResourceRecord> authorities;
  std::vector<MdnsResourceRecord> additionals;
};

MdnsHeaderInfo from_dns_header(const MDNS::DnsHeaderHost &h);

// Parse basic DNS header info. Throws std::invalid_argument on malformed input.
MdnsHeaderInfo parse_basic_header(const uint8_t *buf, size_t len);

// Parse a complete DNS message including header, questions, and resource
// records. Throws std::invalid_argument on malformed input.
MdnsParsedMessage parse_full_message(const uint8_t *buf, size_t len);

// Parse DNS questions starting at offset. Returns questions and new offset.
std::pair<std::vector<MdnsQuestion>, size_t>
parse_questions(const uint8_t *buf, size_t len, size_t offset, uint16_t count);
