#include <cassert>
#include <iostream>
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"

static ResourceRecordView make_view(const std::string& owner,
                                    uint16_t type,
                                    const std::string& rdata) {
    return ResourceRecordView{owner, type, rdata, 120, 1000};
}

void test_meta_ptr_relevant() {
    const auto rec = make_view(MdnsNames::kDnsSdMetaService, DnsType::PTR, "_airplay._tcp.local");
    assert(MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ meta PTR relevant passed" << std::endl;
}

void test_service_instance_ptr_relevant() {
    const auto rec = make_view("_airplay._tcp.local", DnsType::PTR,
                               "Living Room._airplay._tcp.local");
    assert(MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ service instance PTR relevant passed" << std::endl;
}

void test_arpa_ptr_excluded() {
    const auto rec = make_view("79.128.10.in-addr.arpa", DnsType::PTR, "host.example");
    assert(!MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ arpa PTR excluded passed" << std::endl;
}

void test_kerberos_txt_excluded() {
    const auto rec = make_view("_kerberos.khuhlinas-macbook-pro-7.local", DnsType::TXT, "txtvers=1");
    assert(!MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ kerberos TXT excluded passed" << std::endl;
}

void test_srv_instance_relevant() {
    const auto rec = make_view("Living Room._airplay._tcp.local", DnsType::SRV,
                               "priority=0 weight=0 port=7000 target=tv.local");
    assert(MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ SRV instance relevant passed" << std::endl;
}

void test_local_a_relevant() {
    const auto rec = make_view("tv.local", DnsType::A, "10.0.0.5");
    assert(MdnsNames::is_audit_relevant_record(rec));
    std::cout << "  ✓ local A relevant passed" << std::endl;
}

void test_service_type_from_name() {
    const std::string instance = "Living Room._airplay._tcp.local";
    assert(MdnsNames::service_type_from_name(instance) == "_airplay._tcp.local");
    assert(MdnsNames::service_type_from_name("_airplay._tcp.local") == "_airplay._tcp.local");
    assert(MdnsNames::service_type_from_name("tv.local").empty());
    std::cout << "  ✓ service_type_from_name passed" << std::endl;
}

int main() {
    std::cout << "Running MdnsNames tests..." << std::endl << std::endl;
    try {
        test_meta_ptr_relevant();
        test_service_instance_ptr_relevant();
        test_arpa_ptr_excluded();
        test_kerberos_txt_excluded();
        test_srv_instance_relevant();
        test_local_a_relevant();
        test_service_type_from_name();
        std::cout << std::endl << "✓ All MdnsNames tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "✗ MdnsNames test failed: " << ex.what() << std::endl;
        return 1;
    }
}
