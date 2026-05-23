#include "DiscoveryEngine.hpp"
#include "MDNSDefinitions.hpp"
#include "discovery/MdnsNames.hpp"
#include "mdns/DnsTypes.hpp"
#include "mdns/QueryBuilder.hpp"
#include "util/InterfaceInfo.hpp"
#include "util/AgentDebugLog.hpp"
#include <chrono>
#include <cerrno>
#include <sstream>
#include <system_error>

DiscoveryEngine::DiscoveryEngine(std::shared_ptr<Logger> logger_)
    : socket(std::make_unique<NetworkSocket>()),
      logger(std::move(logger_)),
      packet_interpreter(std::make_unique<MdnsPacketInterpreter>(logger)),
      probe_planner_(std::make_unique<MdnsProbePlanner>()),
      running(false) {}

void DiscoveryEngine::set_probe_planner_config(MdnsProbePlannerConfig config) {
    probe_planner_ = std::make_unique<MdnsProbePlanner>(config);
}

bool DiscoveryEngine::start(const std::string& interface_name) {
    std::string iface_ip;
    if (!interface_name.empty()) {
        iface_ip = ipv4_for_interface(interface_name);
        if (iface_ip.empty()) {
            logger->error("Interface not found or has no IPv4: " + interface_name);
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
        bound << " on interface " << interface_name << " (" << iface_ip << ")";
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

void DiscoveryEngine::broadcast_meta_browse() {
    QueryBuilder qb(0);
    qb.add_question(MdnsNames::kDnsSdMetaService, DnsType::PTR, DnsClass::IN);
    auto query = qb.build();

    try {
        socket->send_packet(query, MDNS::MULTICAST_GROUP, MDNS::PORT);
        logger->debug("Broadcasted meta service discovery query.");
    } catch (const std::exception& ex) {
        logger->error(std::string("Failed to broadcast meta browse: ") + ex.what());
        error_occurred_.store(true);
    }
}

void DiscoveryEngine::send_probe_batch(const MdnsProbeBatch& batch) {
    if (batch.empty()) return;

    QueryBuilder qb(0);
    for (const auto& req : batch) {
        qb.add_question(req.qname, req.qtype, DnsClass::IN);
    }
    auto query = qb.build();

    try {
        socket->send_packet(query, MDNS::MULTICAST_GROUP, MDNS::PORT);
        std::ostringstream oss;
        oss << "Probe query";
        for (const auto& req : batch) {
            oss << " " << req.qname << "/" << req.qtype;
        }
        logger->debug(oss.str());
    } catch (const std::exception& ex) {
        logger->error(std::string("Failed to send probe query: ") + ex.what());
        error_occurred_.store(true);
    }
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
                // #region agent log
                {
                    std::ostringstream dj;
                    dj << "{\"src\":\"" << sender_ip << "\",\"bytes\":" << n << "}";
                    agent_debug_log("H3", "DiscoveryEngine.cpp:receiver_loop",
                                    "udp_packet_received", dj.str());
                }
                // #endregion
                std::lock_guard<std::mutex> lk(queue_mutex);
                if (packet_queue.size() >= kMaxPacketQueueSize) {
                    packet_queue.pop_front();
                    ++dropped_packets_;
                }
                packet_queue.emplace_back(buffer, sender_ip);
                queue_cv.notify_one();
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
        std::unique_lock<std::mutex> lk(queue_mutex);
        queue_cv.wait(lk, [this] { return !packet_queue.empty() || !running.load(); });
        if (!running.load() && packet_queue.empty()) break;
        auto item = std::move(packet_queue.front());
        packet_queue.pop_front();
        lk.unlock();

        const auto& buffer = item.first;
        const auto& src_ip = item.second;
        try {
            auto msg = packet_interpreter->decode_packet(buffer);

            if (!MdnsPacketInterpreter::is_dns_response(msg)) {
                packet_interpreter->log_ignored_query(src_ip);
                continue;
            }

            packet_interpreter->log_packet_summary(src_ip, buffer, msg);

            auto ev = packet_interpreter->create_discovery_event(src_ip, buffer, msg);
            if (ev.records.empty() && msg.header.answer_rrs > 0) {
                packet_interpreter->log_undisplayable_response(src_ip, msg);
            }
            if (probe_planner_) {
                probe_planner_->observe(ev);
            }
            if (sink && !ev.records.empty()) {
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
    dropped_packets_ = 0;
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
    queue_cv.notify_all();

    if (receiver_thread.joinable()) receiver_thread.join();
    if (poller_thread.joinable()) poller_thread.join();
    if (worker_thread.joinable()) worker_thread.join();

    if (dropped_packets_ > 0) {
        logger->warn("Dropped " + std::to_string(dropped_packets_) +
                     " packets due to queue limit (" + std::to_string(kMaxPacketQueueSize) + ")");
    }

    logger->info("Discovery loop exiting.");
    return setup_ok && !error_occurred_.load();
}
