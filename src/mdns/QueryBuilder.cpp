#include "mdns/QueryBuilder.hpp"
#include "mdns/DnsProtocol.hpp"
#include <cstring>

QueryBuilder::QueryBuilder(uint16_t tid) : transaction_id(tid) {
    buffer.reserve(256);
    for (size_t i = 0; i < DnsProtocol::kHeaderSize; ++i) {
        buffer.push_back(0);
    }
}

void QueryBuilder::append_label(const std::string& label) {
    buffer.push_back(static_cast<uint8_t>(label.length()));
    for (char c : label) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

QueryBuilder& QueryBuilder::add_question(const std::string& name, uint16_t type, uint16_t class_code) {
    size_t pos = 0;
    while (pos < name.length()) {
        size_t dot_pos = name.find('.', pos);
        if (dot_pos == std::string::npos) {
            std::string label = name.substr(pos);
            if (!label.empty()) {
                append_label(label);
            }
            break;
        }
        std::string label = name.substr(pos, dot_pos - pos);
        if (!label.empty()) {
            append_label(label);
        }
        pos = dot_pos + 1;
    }

    buffer.push_back(DnsProtocol::kQnameTerminator);

    buffer.push_back(static_cast<uint8_t>(type >> 8));
    buffer.push_back(static_cast<uint8_t>(type & 0xFF));

    buffer.push_back(static_cast<uint8_t>(class_code >> 8));
    buffer.push_back(static_cast<uint8_t>(class_code & 0xFF));

    ++question_count_;
    return *this;
}

std::vector<uint8_t> QueryBuilder::build() const {
    std::vector<uint8_t> result = buffer;

    result[0] = static_cast<uint8_t>(transaction_id >> 8);
    result[1] = static_cast<uint8_t>(transaction_id & 0xFF);

    result[2] = 0x00;
    result[3] = 0x00;

    result[4] = static_cast<uint8_t>(question_count_ >> 8);
    result[5] = static_cast<uint8_t>(question_count_ & 0xFF);

    for (size_t i = 6; i < DnsProtocol::kHeaderSize; ++i) {
        result[i] = 0x00;
    }

    return result;
}
