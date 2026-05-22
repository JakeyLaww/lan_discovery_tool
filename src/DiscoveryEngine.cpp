#include "DiscoveryEngine.hpp"
#include "MDNSDefinitions.hpp"
#include "mdns/QueryBuilder.hpp"
#include <chrono>
#include <cerrno>
#include <system_error>

DiscoveryEngine::DiscoveryEngine(std::shared_ptr<Logger> logger_)
    : socket(std::make_unique<NetworkSocket>()),
      logger(std::move(logger_)),
      packet_interpreter(std::make_unique<MdnsPacketInterpreter>(logger)),
      running(false) {}

bool DiscoveryEngine::start() {
    try {
        socket->bind_multicast(MDNS::PORT, MDNS::MULTICAST_GROUP);
    } catch (const std::exception& ex) {
        logger->error(std::string("Failed to bind to mDNS multicast group: ") + ex.what());
        return false;
    }
    logger->info(std::string("Engine active. Bound to ") + MDNS::MULTICAST_GROUP + ":" +
                  std::to_string(MDNS::PORT));
    return true;
}

void DiscoveryEngine::broadcast_query() {
    QueryBuilder qb(0);
    qb.add_question("_services._dns-sd._udp.local", 12, 1);
    auto query = qb.build();

    try {
        socket->send_packet(query, MDNS::MULTICAST_GROUP, MDNS::PORT);
        logger->debug("Broadcasted service discovery query.");
    } catch (const std::exception& ex) {
        logger->error(std::string("Failed to broadcast query: ") + ex.what());
        error_occurred_.store(true);
    }
}

void DiscoveryEngine::stop() {
    running.store(false);
}

bool DiscoveryEngine::is_running() const noexcept {
    return running.load();
}

void DiscoveryEngine::set_event_sink(EventSinkPtr s) {
    sink = std::move(s);
}

void DiscoveryEngine::set_verbose(bool verbose) {
    packet_interpreter->set_verbose(verbose);
}

void DiscoveryEngine::receiver_loop() {
    std::vector<uint8_t> buffer;
    std::string sender_ip;
    while (running.load()) {
        try {
            ssize_t n = socket->receive_packet(buffer, sender_ip);
            if (n > 0) {
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

            auto ev = packet_interpreter->create_discovery_event(src_ip, msg);
            if (ev.records.empty() && msg.header.answer_rrs > 0) {
                packet_interpreter->log_undisplayable_response(src_ip, msg);
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
        try {
            broadcast_query();
        } catch (const std::exception& ex) {
            logger->error(std::string("Poller failed to broadcast query: ") + ex.what());
            error_occurred_.store(true);
        }
        std::this_thread::sleep_for(interval);
        if (!running.load()) break;
    }
}

bool DiscoveryEngine::run(uint32_t poll_interval_ms, std::function<bool()> shutdown_requested) {
    if (running.load()) {
        logger->warn("Engine is already running");
        return false;
    }

    if (!start()) {
        return false;
    }

    error_occurred_.store(false);
    dropped_packets_ = 0;
    socket->set_receive_timeout(1000);
    running.store(true);

    logger->info("Starting receiver, worker, and poller threads...");

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
