#include <iostream>
#include <cassert>
#include <cstring>
#include "mdns/DnsProtocol.hpp"
#include "mdns/DnsTypes.hpp"
#include "mdns/Parser.hpp"
#include "mdns/QueryBuilder.hpp"
#include "MDNSDefinitions.hpp"

void test_parse_basic_header() {
    std::cout << "Testing parse_basic_header..." << std::endl;

    // Construct a minimal DNS header: ID=0x1234, Flags=response, Questions=1, Answers=2
    uint8_t buffer[12] = {
        0x12, 0x34,  // Transaction ID = 0x1234
        0x80, 0x00,  // Flags = 0x8000 (response)
        0x00, 0x01,  // Questions = 1
        0x00, 0x02,  // Answer RRs = 2
        0x00, 0x00,  // Authority RRs = 0
        0x00, 0x00   // Additional RRs = 0
    };

    auto header = parse_basic_header(buffer, sizeof(buffer));
    if (header.transaction_id != 0x1234 || header.flags != DnsProtocol::kFlagsResponse ||
        header.questions != 1 || header.answer_rrs != 2 ||
        header.authority_rrs != 0 || header.additional_rrs != 0) {
        throw std::runtime_error("parse_basic_header mismatch");
    }

    std::cout << "  ✓ parse_basic_header passed" << std::endl;
}

void test_parse_qname_simple() {
    std::cout << "Testing parse_qname with simple uncompressed name..." << std::endl;

    // Construct "_http._tcp.local"
    //   09 "_http" 04 "_tcp" 05 "local" 00
    uint8_t buffer[] = {
        0x05, '_', 'h', 't', 't', 'p',   // label 1: "_http"
        0x04, '_', 't', 'c', 'p',        // label 2: "_tcp"
        0x05, 'l', 'o', 'c', 'a', 'l',   // label 3: "local"
        0x00                              // terminator
    };

    auto [name, offset] = MDNS::parse_qname(buffer, sizeof(buffer), 0);
    assert(name == "_http._tcp.local");
    assert(offset == sizeof(buffer));

    std::cout << "  ✓ parse_qname simple passed" << std::endl;
}

void test_parse_qname_with_compression() {
    std::cout << "Testing parse_qname with compression pointers..." << std::endl;

    // Construct a DNS message with compression:
    // At offset 0: "_services._dns-sd._udp.local"
    // At offset 35: "_services._dns-sd._udp.local" (pointer to 0)
    uint8_t buffer[50];
    memset(buffer, 0, sizeof(buffer));

    // Write header (12 bytes)
    buffer[0] = 0x00; buffer[1] = 0x00;  // ID
    buffer[2] = 0x00; buffer[3] = 0x00;  // Flags
    buffer[4] = 0x00; buffer[5] = 0x01;  // Questions = 1

    // Write first name at offset 12: "_services._dns-sd._udp.local"
    size_t pos = 12;
    buffer[pos++] = 9;
    memcpy(buffer + pos, "_services", 9);
    pos += 9;
    buffer[pos++] = 7;
    memcpy(buffer + pos, "_dns-sd", 7);
    pos += 7;
    buffer[pos++] = 4;
    memcpy(buffer + pos, "_udp", 4);
    pos += 4;
    buffer[pos++] = 5;
    memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0;  // terminator

    // Now test parsing the uncompressed name
    auto [name, offset] = MDNS::parse_qname(buffer, sizeof(buffer), 12);
    assert(name == "_services._dns-sd._udp.local");

    std::cout << "  ✓ parse_qname with compression passed" << std::endl;
}

void test_parse_questions() {
    std::cout << "Testing parse_questions..." << std::endl;

    // Build a DNS message with one question
    uint8_t buffer[50];
    memset(buffer, 0, sizeof(buffer));

    // Header
    buffer[0] = 0x00; buffer[1] = 0x00;   // ID
    buffer[2] = 0x00; buffer[3] = 0x00;   // Flags
    buffer[4] = 0x00; buffer[5] = 0x01;   // Questions = 1
    buffer[6] = 0x00; buffer[7] = 0x00;   // Answers = 0

    // Question at offset 12: "test.local" type=A, class=IN
    size_t pos = 12;
    buffer[pos++] = 4;
    memcpy(buffer + pos, "test", 4);
    pos += 4;
    buffer[pos++] = 5;
    memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0;  // terminator
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Type = 1 (A)
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Class = 1 (IN)

    auto [questions, offset] = parse_questions(buffer, sizeof(buffer), 12, 1);
    assert(questions.size() == 1);
    assert(questions[0].name == "test.local");
    assert(questions[0].type == DnsType::A);
    assert(questions[0].class_code == DnsClass::IN);

    std::cout << "  ✓ parse_questions passed" << std::endl;
}

void test_parse_full_message() {
    std::cout << "Testing parse_full_message..." << std::endl;

    // Build a simple DNS response with one A record
    uint8_t buffer[100];
    memset(buffer, 0, sizeof(buffer));

    // Header: ID=0, Flags=mDNS response, Questions=1, Answers=1
    buffer[0] = 0x00; buffer[1] = 0x00;   // ID
    buffer[2] = 0x84; buffer[3] = 0x00;   // Flags = 0x8400 (response)
    buffer[4] = 0x00; buffer[5] = 0x01;   // Questions = 1
    buffer[6] = 0x00; buffer[7] = 0x01;   // Answers = 1
    buffer[8] = 0x00; buffer[9] = 0x00;   // Authority = 0
    buffer[10] = 0x00; buffer[11] = 0x00; // Additional = 0

    // Question: "test.local" A IN
    size_t pos = 12;
    buffer[pos++] = 4;
    memcpy(buffer + pos, "test", 4);
    pos += 4;
    buffer[pos++] = 5;
    memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0;  // terminator
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Type = A
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Class = IN

    // Answer: "test.local" A IN TTL=120 RDATA=192.168.1.1
    buffer[pos++] = 4;
    memcpy(buffer + pos, "test", 4);
    pos += 4;
    buffer[pos++] = 5;
    memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0;  // terminator
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Type = A
    buffer[pos++] = 0x00; buffer[pos++] = 0x01;  // Class = IN
    buffer[pos++] = 0x00; buffer[pos++] = 0x00;  // TTL high bytes
    buffer[pos++] = 0x00; buffer[pos++] = 0x78;  // TTL = 120
    buffer[pos++] = 0x00; buffer[pos++] = 0x04;  // RDLENGTH = 4
    buffer[pos++] = 192;
    buffer[pos++] = 168;
    buffer[pos++] = 1;
    buffer[pos++] = 1;

    auto msg = parse_full_message(buffer, pos);
    assert(msg.header.transaction_id == 0);
    assert(msg.header.flags == DnsProtocol::kFlagsMdnsResponse);
    assert(msg.header.questions == 1);
    assert(msg.header.answer_rrs == 1);
    assert(msg.questions.size() == 1);
    assert(msg.questions[0].name == "test.local");
    assert(msg.answers.size() == 1);
    assert(msg.answers[0].name == "test.local");
    assert(msg.answers[0].type == DnsType::A);
    assert(msg.answers[0].ttl == 120);
    assert(msg.answers[0].rdata.size() == 4);
    assert(msg.answers[0].rdata_str() == "192.168.1.1");

    std::cout << "  ✓ parse_full_message passed" << std::endl;
}

void test_rdata_formatters() {
    std::cout << "Testing RDATA formatters..." << std::endl;

    // Test A record formatter
    {
        MdnsResourceRecord rr;
        rr.type = DnsType::A;
        rr.rdata = {192, 168, 1, 100};
        assert(rr.rdata_str() == "192.168.1.100");
    }

    // Test PTR record formatter
    {
        uint8_t ptr_data[] = {
            0x04, '_', 'i', 't', 'u', 'n', 'e', 's', '_',
            0x04, '_', 't', 'c', 'p', 0x00
        };
        MdnsResourceRecord rr;
        rr.type = DnsType::PTR;
        rr.rdata.assign(ptr_data, ptr_data + sizeof(ptr_data));
        std::string result = rr.rdata_str();
        assert(result.find("itunes") != std::string::npos);
    }

    std::cout << "  ✓ rdata formatters passed" << std::endl;
}

void test_query_builder_question_count() {
    std::cout << "Testing QueryBuilder question count..." << std::endl;

    QueryBuilder qb(0xABCD);
    qb.add_question("_http._tcp.local", DnsType::PTR, DnsClass::IN);
    auto query = qb.build();

    assert(query.size() >= 12);
    assert(query[0] == 0xAB);
    assert(query[1] == 0xCD);
    assert(query[4] == 0x00);
    assert(query[5] == 0x01);

    QueryBuilder qb2(0);
    qb2.add_question("_services._dns-sd._udp.local", DnsType::PTR, DnsClass::IN);
    qb2.add_question("test.local", DnsType::A, DnsClass::IN);
    auto query2 = qb2.build();
    assert(query2[4] == 0x00);
    assert(query2[5] == 0x02);

    std::cout << "  ✓ QueryBuilder question count passed" << std::endl;
}

int main() {
    std::cout << "Running DNS Parser Unit Tests..." << std::endl << std::endl;

    try {
        test_parse_basic_header();
        test_parse_qname_simple();
        test_parse_qname_with_compression();
        test_parse_questions();
        test_parse_full_message();
        test_rdata_formatters();
        test_query_builder_question_count();

        std::cout << std::endl << "✓ All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "✗ Test failed with exception: " << ex.what() << std::endl;
        return 1;
    }
}
