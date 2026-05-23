#include <cassert>
#include <iostream>
#include <stdexcept>
#include "discovery/DeviceRegistry.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"

static DiscoveryEvent make_event(const std::string& src_ip, uint64_t ts) {
    DiscoveryEvent ev;
    ev.src_ip = src_ip;
    ev.timestamp_ms = ts;
    return ev;
}

static ResourceRecordView ptr_meta(const std::string& service, uint64_t ts, uint32_t ttl = 4500) {
    return ResourceRecordView{
        MdnsNames::kDnsSdMetaService,
        DnsType::PTR,
        service,
        ttl,
        ts};
}

static ResourceRecordView ptr_instance(const std::string& service,
                                       const std::string& instance,
                                       uint64_t ts) {
    return ResourceRecordView{service, DnsType::PTR, instance, 120, ts};
}

static ResourceRecordView srv_record(const std::string& instance,
                                     const std::string& target,
                                     uint64_t ts) {
    return ResourceRecordView{
        instance,
        DnsType::SRV,
        "priority=0 weight=0 port=7000 target=" + target,
        120,
        ts};
}

static void require_apply(DeviceRegistry& registry, const DiscoveryEvent& ev) {
    if (!registry.apply(ev)) {
        throw std::runtime_error("apply returned false for " + ev.src_ip);
    }
}

static const DeviceSnapshot& require_snap(const DeviceRegistry& registry,
                                          const std::string& src_ip) {
    const DeviceSnapshot* snap = registry.find(src_ip);
    if (!snap) {
        throw std::runtime_error("missing device snapshot for " + src_ip);
    }
    return *snap;
}

void test_meta_ptr_adds_service_type() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 1000);
    ev.records.push_back(ptr_meta("_airplay._tcp.local", 1000));

    require_apply(registry, ev);
    if (require_snap(registry, "10.0.0.5").service_types.count("_airplay._tcp.local") != 1) {
        throw std::runtime_error("meta PTR did not add service type");
    }

    std::cout << "  ✓ meta PTR adds service type passed" << std::endl;
}

void test_service_ptr_adds_instance() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 2000);
    ev.records.push_back(ptr_instance(
        "_airplay._tcp.local", "Living Room._airplay._tcp.local", 2000));

    require_apply(registry, ev);
    if (require_snap(registry, "10.0.0.5").instances.count("Living Room._airplay._tcp.local") != 1) {
        throw std::runtime_error("service PTR did not add instance");
    }

    std::cout << "  ✓ service PTR adds instance passed" << std::endl;
}

void test_srv_sets_host() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 3000);
    ev.records.push_back(srv_record(
        "Living Room._airplay._tcp.local", "tv.local", 3000));

    require_apply(registry, ev);
    if (require_snap(registry, "10.0.0.5").mdns_host != "tv.local") {
        throw std::runtime_error("SRV did not set host");
    }

    std::cout << "  ✓ SRV sets host passed" << std::endl;
}

void test_apply_false_on_duplicate() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 4000);
    ev.records.push_back(ptr_meta("_http._tcp.local", 4000));

    require_apply(registry, ev);
    if (registry.apply(ev)) {
        throw std::runtime_error("duplicate apply should return false");
    }

    std::cout << "  ✓ apply false on duplicate passed" << std::endl;
}

void test_apply_false_on_timestamp_only() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 5000);
    ev.records.push_back(ptr_meta("_http._tcp.local", 5000));

    require_apply(registry, ev);

    auto later = make_event("10.0.0.5", 6000);
    later.records.push_back(ptr_meta("_http._tcp.local", 6000));
    if (registry.apply(later)) {
        throw std::runtime_error("timestamp-only apply should return false");
    }

    if (require_snap(registry, "10.0.0.5").last_seen_ms != 6000) {
        throw std::runtime_error("last_seen_ms was not updated");
    }

    std::cout << "  ✓ apply false on timestamp only passed" << std::endl;
}

void test_instance_ptr_adds_service_type() {
    DeviceRegistry registry;
    auto ev = make_event("10.0.0.5", 7000);
    ev.records.push_back(ptr_instance(
        "_device-info._tcp.local", "Mac._device-info._tcp.local", 7000));

    require_apply(registry, ev);
    if (require_snap(registry, "10.0.0.5").service_types.count("_device-info._tcp.local") != 1) {
        throw std::runtime_error("instance PTR did not add service type");
    }

    std::cout << "  ✓ instance PTR adds service type passed" << std::endl;
}

void test_summary_contains_services() {
    DeviceRegistry registry;
    auto ev = make_event("10.128.79.93", 5000);
    ev.records.push_back(ptr_meta("_airplay._tcp.local", 5000));
    ev.records.push_back(ptr_meta("_display._tcp.local", 5000));

    require_apply(registry, ev);
    const std::string summary = registry.format_summary("10.128.79.93");
    if (summary.find("device src=10.128.79.93") == std::string::npos ||
        summary.find("_airplay._tcp.local") == std::string::npos ||
        summary.find("_display._tcp.local") == std::string::npos) {
        throw std::runtime_error("summary missing expected services");
    }

    std::cout << "  ✓ summary contains services passed" << std::endl;
}

int main() {
    std::cout << "Running device registry tests..." << std::endl << std::endl;
    try {
        test_meta_ptr_adds_service_type();
        test_service_ptr_adds_instance();
        test_srv_sets_host();
        test_apply_false_on_duplicate();
        test_apply_false_on_timestamp_only();
        test_instance_ptr_adds_service_type();
        test_summary_contains_services();
        std::cout << std::endl << "✓ All device registry tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "✗ Device registry test failed: " << ex.what() << std::endl;
        return 1;
    }
}
