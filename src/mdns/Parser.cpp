#include "mdns/Parser.hpp"
#include "mdns/MdnsMessageDecoder.hpp"
#include "mdns/RDataFormatter.hpp"
#include "MDNSDefinitions.hpp"

MdnsHeaderInfo parse_basic_header(const uint8_t* buf, size_t len) {
    auto h = MDNS::parse_dns_header(buf, len);
    MdnsHeaderInfo info;
    info.transaction_id = h.transaction_id;
    info.flags = h.flags;
    info.questions = h.questions;
    info.answer_rrs = h.answer_rrs;
    info.authority_rrs = h.authority_rrs;
    info.additional_rrs = h.additional_rrs;
    return info;
}

std::pair<std::vector<MdnsQuestion>, size_t> parse_questions(
    const uint8_t* buf, size_t len, size_t offset, uint16_t count) {
    std::vector<MdnsQuestion> questions;
    size_t pos = offset;

    for (uint16_t i = 0; i < count; ++i) {
        if (pos >= len) {
            throw std::invalid_argument("Questions truncated at buffer end");
        }

        // Parse QNAME
        auto [name, new_pos] = MDNS::parse_qname(buf, len, pos);
        pos = new_pos;

        // Parse TYPE (2 bytes)
        if (pos + 2 > len) {
            throw std::invalid_argument("Question TYPE field truncated");
        }
        uint16_t type = MDNS::read_u16_be(buf + pos);
        pos += 2;

        // Parse CLASS (2 bytes)
        if (pos + 2 > len) {
            throw std::invalid_argument("Question CLASS field truncated");
        }
        uint16_t class_code = MDNS::read_u16_be(buf + pos);
        pos += 2;

        questions.push_back({name, type, class_code});
    }

    return {questions, pos};
}

MdnsParsedMessage parse_full_message(const uint8_t* buf, size_t len) {
    MdnsMessageDecoder decoder(buf, len);
    return decoder.decode();
}

std::string MdnsResourceRecord::rdata_str() const {
    auto formatter = get_rdata_formatter(type);
    return formatter->format(rdata);
}
