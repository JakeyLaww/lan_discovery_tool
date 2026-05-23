#include <cassert>
#include <iostream>
#include "discovery/RecordLabels.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"

void test_srv_service_label_uses_instance() {
    const ResourceRecordView rec{
        "Living Room._airplay._tcp.local",
        DnsType::SRV,
        "priority=0 weight=0 port=7000 target=tv.local",
        120,
        1000};

    assert(service_label(rec) == "Living Room._airplay._tcp.local");
    assert(host_label(rec) == "tv.local");
    std::cout << "  ✓ SRV service label uses instance passed" << std::endl;
}

int main() {
    std::cout << "Running record labels tests..." << std::endl << std::endl;
    try {
        test_srv_service_label_uses_instance();
        std::cout << std::endl << "✓ All record labels tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "✗ Record labels test failed: " << ex.what() << std::endl;
        return 1;
    }
}
