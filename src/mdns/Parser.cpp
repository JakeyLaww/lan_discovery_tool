#include "mdns/Parser.hpp"
#include "MDNSDefinitions.hpp"
#include "mdns/MdnsMessageDecoder.hpp"
#include "mdns/RDataFormatter.hpp"
#include "util/BinaryStreamReader.hpp"
#include "util/BufferBoundsValidator.hpp"

MdnsHeaderInfo from_dns_header(const MDNS::DnsHeaderHost &h) {
  MdnsHeaderInfo info;
  info.transaction_id = h.transaction_id;
  info.flags = h.flags;
  info.questions = h.questions;
  info.answer_rrs = h.answer_rrs;
  info.authority_rrs = h.authority_rrs;
  info.additional_rrs = h.additional_rrs;
  return info;
}

MdnsHeaderInfo parse_basic_header(const uint8_t *buf, size_t len) {
  return from_dns_header(MDNS::parse_dns_header(buf, len));
}

std::pair<std::vector<MdnsQuestion>, size_t>
parse_questions(const uint8_t *buf, size_t len, size_t offset, uint16_t count) {
  std::vector<MdnsQuestion> questions;
  size_t pos = offset;

  for (uint16_t i = 0; i < count; ++i) {
    if (pos >= len) {
      throw std::invalid_argument("Questions truncated at buffer end");
    }

    auto [name, new_pos] = MDNS::parse_qname(buf, len, pos);
    pos = new_pos;

    BufferBoundsValidator::ensure_readable(pos, 2, len, "Question TYPE field");
    uint16_t type = BinaryStreamReader::read_u16_be(buf + pos);
    pos += 2;

    BufferBoundsValidator::ensure_readable(pos, 2, len, "Question CLASS field");
    uint16_t class_code = BinaryStreamReader::read_u16_be(buf + pos);
    pos += 2;

    questions.push_back({name, type, class_code});
  }

  return {questions, pos};
}

MdnsParsedMessage parse_full_message(const uint8_t *buf, size_t len) {
  MdnsMessageDecoder decoder(buf, len);
  return decoder.decode();
}

std::string MdnsResourceRecord::rdata_str(const uint8_t *message,
                                          size_t message_len) const {
  RDataFormatContext ctx;
  ctx.message = message;
  ctx.message_len = message_len;
  ctx.rdata_offset = rdata_offset;
  ctx.rdata = &rdata;
  auto formatter = get_rdata_formatter(type);
  return formatter->format(ctx);
}
