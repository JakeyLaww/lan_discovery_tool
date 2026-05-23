#include "mdns/MdnsMessageDecoder.hpp"
#include "MDNSDefinitions.hpp"
#include "util/BinaryStreamReader.hpp"
#include "util/BufferBoundsValidator.hpp"

MdnsMessageDecoder::MdnsMessageDecoder(const uint8_t *buf, size_t len)
    : buffer(buf), buffer_len(len) {}

MdnsResourceRecord MdnsMessageDecoder::parse_record_impl(size_t &pos) {
  if (pos >= buffer_len) {
    throw std::invalid_argument("Resource record offset out of bounds");
  }

  // Parse NAME (QNAME)
  auto [name, new_pos] = MDNS::parse_qname(buffer, buffer_len, pos);
  pos = new_pos;

  // Parse TYPE (2 bytes)
  BufferBoundsValidator::ensure_readable(pos, 2, buffer_len, "RR TYPE field");
  uint16_t type = BinaryStreamReader::read_u16_be(buffer + pos);
  pos += 2;

  // Parse CLASS (2 bytes)
  BufferBoundsValidator::ensure_readable(pos, 2, buffer_len, "RR CLASS field");
  uint16_t class_code = BinaryStreamReader::read_u16_be(buffer + pos);
  pos += 2;

  // Parse TTL (4 bytes)
  BufferBoundsValidator::ensure_readable(pos, 4, buffer_len, "RR TTL field");
  uint32_t ttl = BinaryStreamReader::read_u32_be(buffer + pos);
  pos += 4;

  // Parse RDLENGTH (2 bytes)
  BufferBoundsValidator::ensure_readable(pos, 2, buffer_len,
                                         "RR RDLENGTH field");
  uint16_t rdlength = BinaryStreamReader::read_u16_be(buffer + pos);
  pos += 2;

  // Parse RDATA
  BufferBoundsValidator::ensure_readable(pos, rdlength, buffer_len, "RR RDATA");
  const size_t rdata_offset = pos;
  std::vector<uint8_t> rdata(buffer + pos, buffer + pos + rdlength);
  pos += rdlength;

  return MdnsResourceRecord{name, type, class_code, ttl, rdata, rdata_offset};
}

std::vector<MdnsResourceRecord>
MdnsMessageDecoder::parse_record_section(uint16_t count, size_t &pos) {
  std::vector<MdnsResourceRecord> records;
  for (uint16_t i = 0; i < count; ++i) {
    records.push_back(parse_record_impl(pos));
  }
  return records;
}

MdnsParsedMessage MdnsMessageDecoder::decode() {
  auto header = from_dns_header(MDNS::parse_dns_header(buffer, buffer_len));

  size_t pos = 12; // Header is always 12 bytes
  MdnsParsedMessage msg{header, {}, {}, {}, {}};

  // Parse questions section
  if (header.questions > 0) {
    auto [questions, new_pos] =
        parse_questions(buffer, buffer_len, pos, header.questions);
    msg.questions = questions;
    pos = new_pos;
  }

  // Parse answer records section (replaces Loop 1 of old code)
  msg.answers = parse_record_section(header.answer_rrs, pos);

  // Parse authority records section (replaces Loop 2 of old code)
  msg.authorities = parse_record_section(header.authority_rrs, pos);

  // Parse additional records section (replaces Loop 3 of old code)
  msg.additionals = parse_record_section(header.additional_rrs, pos);

  return msg;
}
