#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "core/MdnsPacketInterpreter.hpp"
#include "discovery/DiscoveryTypes.hpp"
#include "log/ThreadSafeLogger.hpp"
#include "mdns/DnsTypes.hpp"

void test_is_dns_response() {
    MdnsParsedMessage query;
    query.header.flags = 0x0000;
    assert(!MdnsPacketInterpreter::is_dns_response(query));

    MdnsParsedMessage response;
    response.header.flags = 0x8400;
    assert(MdnsPacketInterpreter::is_dns_response(response));

    std::cout << "  ✓ is_dns_response passed" << std::endl;
}

void test_create_discovery_event_from_ptr_response() {
    // DNS response: 1 answer PTR _services._dns-sd._udp.local -> _airplay._tcp.local
    uint8_t buffer[128];
    std::memset(buffer, 0, sizeof(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x84;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x01;
    buffer[8] = 0x00;
    buffer[9] = 0x00;
    buffer[10] = 0x00;
    buffer[11] = 0x00;

    size_t pos = 12;
    buffer[pos++] = 9;
    std::memcpy(buffer + pos, "_services", 9);
    pos += 9;
    buffer[pos++] = 7;
    std::memcpy(buffer + pos, "_dns-sd", 7);
    pos += 7;
    buffer[pos++] = 4;
    std::memcpy(buffer + pos, "_udp", 4);
    pos += 4;
    buffer[pos++] = 5;
    std::memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0x00;

    buffer[pos++] = 0x00;
    buffer[pos++] = 0x0c;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x0a;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x0f;

    buffer[pos++] = 9;
    std::memcpy(buffer + pos, "_airplay", 9);
    pos += 9;
    buffer[pos++] = 4;
    std::memcpy(buffer + pos, "_tcp", 4);
    pos += 4;
    buffer[pos++] = 5;
    std::memcpy(buffer + pos, "local", 5);
    pos += 5;
    buffer[pos++] = 0x00;

    auto logger = std::make_shared<StdoutLogger>(LogLevel::Error);
    MdnsPacketInterpreter interpreter(logger);

    std::vector<uint8_t> packet(buffer, buffer + pos);
    auto msg = interpreter.decode_packet(packet);
    assert(MdnsPacketInterpreter::is_dns_response(msg));

    auto ev = interpreter.create_discovery_event("192.168.1.10", msg);
    assert(!ev.records.empty());
    assert(ev.records[0].type == DnsType::PTR);
    assert(ev.records[0].rdata_text.find("airplay") != std::string::npos);

    const std::string json = to_json(ev);
    assert(json.find("192.168.1.10") != std::string::npos);
    assert(json.find("airplay") != std::string::npos);

    std::cout << "  ✓ create_discovery_event_from_ptr_response passed" << std::endl;
}

int main() {
    std::cout << "Running interpreter tests..." << std::endl << std::endl;
    try {
        test_is_dns_response();
        test_create_discovery_event_from_ptr_response();
        std::cout << std::endl << "✓ All interpreter tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "✗ Interpreter test failed: " << ex.what() << std::endl;
        return 1;
    }
}
