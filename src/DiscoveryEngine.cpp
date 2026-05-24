#include "DiscoveryEngine.hpp"
#include "NetworkSocket.hpp"
#include "MDNSDefinitions.hpp"
#include "core/MdnsPacketInterpreter.hpp"
#include "core/PacketReceiveQueue.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"
#include "mdns/QueryBuilder.hpp"
#include "util/InterfaceInfo.hpp"
#include <chrono>
#include <cerrno>
#include <sstream>
#include <system_error>

DiscoveryEngine::DiscoveryEngine(std::shared_ptr<Logger> logger)
    : DiscoveryEngine(std::move(logger), nullptr) {}

DiscoveryEngine::DiscoveryEngine(std::shared_ptr<Logger> logger_,
                                 std::unique_ptr<MdnsPacketInterpreter> interpreter)
    : socket(std::make_unique<NetworkSocket>()),
      logger(std::move(logger_)),
      packet_interpreter(interpreter ? std::move(interpreter)
                                     : std::make_unique<MdnsPacketInterpreter>(logger)),
      probe_planner_(std::make_unique<MdnsProbePlanner>()),
      packet_queue_(std::make_unique<PacketReceiveQueue>(kMaxPacketQueueSize)),
      running(false) {}

DiscoveryEngine::~DiscoveryEngine() = default;

void DiscoveryEngine::set_probe_planner_config(MdnsProbePlannerConfig config) {
    probe_planner_ = std::make_unique<MdnsProbePlanner>(config);
}

bool DiscoveryEngine::start(const std::string& interface_name) {
    std::string bound_iface = interface_name;
    std::string iface_ip;
    if (bound_iface.empty()) {
        if (const auto picked = pick_default_multicast_interface()) {
            bound_iface = picked->name;
            iface_ip = picked->ipv4;
            logger->info("Auto-selected interface " + bound_iface + " (" + iface_ip +
                         ") for multicast");
        }
    } else {
        iface_ip = ipv4_for_interface(bound_iface);
        if (iface_ip.empty()) {
            logger->error("Interface not found or has no IPv4: " + bound_iface);
            return false;
        }
    }

    try {
        socket->bind_multicast(MDNS::PORT, MDNS::MULTICAST_GROUP, iface_ip);
    } catch (const std::exception& ex) {
        logger->error(std::string("Failed to bind to mDNS multicast group: ") + ex.what());
        return false;
    }

    std::ostringstream bound;
    bound << "Engine active. mDNS " << MDNS::MULTICAST_GROUP << ":" << MDNS::PORT;
    if (!iface_ip.empty()) {
        bound << " on interface " << bound_iface << " (" << iface_ip << ")";
    }

    const auto ifaces = list_ipv4_interfaces();
    if (ifaces.empty()) {
        logger->warn("No non-loopback IPv4 interfaces detected.");
    } else {
        bound << " | local:";
        for (const auto& iface : ifaces) {
            bound << " " << iface.name << "=" << iface.ipv4;
        }
    }
    logger->info(bound.str());
    return true;
}

void DiscoveryEngine::send_mdns_questions(const std::vector<MdnsProbeRequest>& questions,
                                          const char* success_log,
                                          const char* error_context) {
    if (questions.empty()) return;

    QueryBuilder qb(0);
    for (const auto& req : questions) {
        qb.add_question(req.qname, req.qtype, DnsClass::IN);
    }
    auto query = qb.build();

    try {
        socket->send_packet(query, MDNS::MULTICAST_GROUP, MDNS::PORT);
        if (success_log != nullptr) {
            logger->debug(success_log);
        }
    } catch (const std::exception& ex) {
        logger->error(std::string(error_context) + ex.what());
        error_occurred_.store(true);
    }
}

void DiscoveryEngine::broadcast_meta_browse() {
    send_mdns_questions({MdnsProbeRequest{MdnsNames::kDnsSdMetaService, DnsType::PTR}},
                        "Broadcasted meta service discovery query.",
                        "Failed to broadcast meta browse: ");
}

void DiscoveryEngine::send_probe_batch(const MdnsProbeBatch& batch) {
    if (batch.empty()) return;

    std::ostringstream probe_log;
    probe_log << "Probe query";
    for (const auto& req : batch) {
        probe_log << " " << req.qname << "/" << req.qtype;
    }
    const std::string probe_log_msg = probe_log.str();
    send_mdns_questions(batch, probe_log_msg.c_str(), "Failed to send probe query: ");
}

void DiscoveryEngine::drain_and_send_probes() {
    if (!probe_planner_) return;

    const auto batches = probe_planner_->drain_ready(0);
    for (const auto& batch : batches) {
        send_probe_batch(batch);
    }
}

void DiscoveryEngine::set_event_sink(EventSinkPtr s) {
    sink = std::move(s);
}

void DiscoveryEngine::set_verbose(bool verbose) {
    packet_interpreter->set_verbose(verbose);
}

void DiscoveryEngine::set_interface(const std::string& interface_name) {
    interface_name_ = interface_name;
}

void DiscoveryEngine::receiver_loop() {
    std::vector<uint8_t> buffer;
    std::string sender_ip;
    while (running.load()) {
        try {
            ssize_t n = socket->receive_packet(buffer, sender_ip);
            if (n > 0) {
                packet_queue_->push(buffer, sender_ip);
            }
        } catch (const std::system_error& ex) {
            int err = ex.code().value();
            if (err == EAGAIN || err == EWOULDBLOCK) continue;
            if (err == EINTR) continue;
            logger->error(std::string("Receiver thread error: ") + ex.what());
            error_occurred_.store(true);
            break;
        }
    }
}

void DiscoveryEngine::worker_loop() {
    while (running.load()) {
        std::vector<uint8_t> buffer;
        std::string src_ip;
        if (!packet_queue_->wait_pop(buffer, src_ip, [this] { return running.load(); })) {
            break;
        }

        try {
            auto msg = packet_interpreter->decode_packet(buffer);

            if (!MdnsPacketInterpreter::is_dns_response(msg)) {
                ++queries_seen_;
                packet_interpreter->log_ignored_query(src_ip);
                continue;
            }
            ++responses_seen_;

            packet_interpreter->log_packet_summary(src_ip, buffer, msg);

            auto ev = packet_interpreter->create_discovery_event(src_ip, buffer, msg);
            if (ev.records.empty() && msg.header.answer_rrs > 0) {
                packet_interpreter->log_undisplayable_response(src_ip, msg);
            }
            if (probe_planner_) {
                probe_planner_->observe(ev);
            }
            if (sink && !ev.records.empty()) {
                ++sink_events_;
                sink->on_event(ev);
            }
        } catch (const std::exception& ex) {
            logger->error(std::string("Worker failed to parse packet: ") + ex.what());
        }
    }
}

void DiscoveryEngine::poller_loop(uint32_t poll_interval_ms) {
    std::chrono::milliseconds interval(poll_interval_ms);
    while (running.load()) {
        broadcast_meta_browse();
        drain_and_send_probes();
        std::this_thread::sleep_for(interval);
        if (!running.load()) break;
    }
}

bool DiscoveryEngine::run(uint32_t poll_interval_ms, std::function<bool()> shutdown_requested) {
    if (running.load()) {
        logger->warn("Engine is already running");
        return false;
    }

    if (!start(interface_name_)) {
        return false;
    }

    error_occurred_.store(false);
    packet_queue_->reset_dropped_count();
    queries_seen_ = 0;
    responses_seen_ = 0;
    sink_events_ = 0;
    socket->set_receive_timeout(1000);
    running.store(true);

    bool setup_ok = true;
    try {
        worker_thread = std::thread(&DiscoveryEngine::worker_loop, this);
        receiver_thread = std::thread(&DiscoveryEngine::receiver_loop, this);
        poller_thread = std::thread(&DiscoveryEngine::poller_loop, this, poll_interval_ms);

        while (running.load()) {
            if (shutdown_requested && shutdown_requested()) {
                logger->info("Shutdown requested.");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } catch (const std::exception& ex) {
        logger->error(std::string("Exception in main loop: ") + ex.what());
        error_occurred_.store(true);
        setup_ok = false;
    }

    running.store(false);
    packet_queue_->notify_all();

    if (receiver_thread.joinable()) receiver_thread.join();
    if (poller_thread.joinable()) poller_thread.join();
    if (worker_thread.joinable()) worker_thread.join();

    const size_t dropped_packets = packet_queue_->dropped_count();
    if (dropped_packets > 0) {
        logger->warn("Dropped " + std::to_string(dropped_packets) +
                     " packets due to queue limit (" + std::to_string(kMaxPacketQueueSize) + ")");
    }

    logger->info("mDNS session: " + std::to_string(responses_seen_) + " responses, " +
                 std::to_string(queries_seen_) + " queries from other hosts, " +
                 std::to_string(sink_events_) + " discovery events emitted");
    if (responses_seen_ == 0 && queries_seen_ > 0) {
        logger->warn(
            "Heard other hosts' mDNS queries but no responses to this scanner. Ensure Bonjour "
            "devices are awake, or pin the LAN NIC with -I eth1 (see startup interfaces).");
    }

    logger->info("Discovery loop exiting.");
    return setup_ok && !error_occurred_.load();
}