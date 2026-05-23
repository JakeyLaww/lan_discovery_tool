#pragma once
#include "Logger.hpp"
#include "EventSink.hpp"
#include "discovery/MdnsProbePlanner.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>
#include <thread>
#include <vector>

class NetworkSocket;
class MdnsPacketInterpreter;
class PacketReceiveQueue;

/**
 * @brief High-level discovery engine that broadcasts mDNS queries and listens for responses.
 */
class DiscoveryEngine {
public:
    static constexpr size_t kMaxPacketQueueSize = 256;

    explicit DiscoveryEngine(std::shared_ptr<Logger> logger);
    DiscoveryEngine(std::shared_ptr<Logger> logger,
                    std::unique_ptr<MdnsPacketInterpreter> interpreter);
    ~DiscoveryEngine();

    void set_event_sink(EventSinkPtr s);
    void set_verbose(bool verbose);
    void set_interface(const std::string& interface_name);
    void set_probe_planner_config(MdnsProbePlannerConfig config);
    bool run(uint32_t poll_interval_ms = 5000, std::function<bool()> shutdown_requested = nullptr);

private:
    bool start(const std::string& interface_name);
    void broadcast_meta_browse();
    void drain_and_send_probes();
    void send_mdns_questions(const std::vector<MdnsProbeRequest>& questions,
                             const char* success_log,
                             const char* error_context);
    void send_probe_batch(const MdnsProbeBatch& batch);

    std::string interface_name_;
    std::unique_ptr<NetworkSocket> socket;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<MdnsPacketInterpreter> packet_interpreter;
    std::unique_ptr<MdnsProbePlanner> probe_planner_;
    std::unique_ptr<PacketReceiveQueue> packet_queue_;
    std::atomic<bool> running{false};
    std::atomic<bool> error_occurred_{false};
    EventSinkPtr sink;

    std::thread receiver_thread;
    std::thread worker_thread;
    std::thread poller_thread;
    size_t queries_seen_{0};
    size_t responses_seen_{0};
    size_t sink_events_{0};

    void receiver_loop();
    void worker_loop();
    void poller_loop(uint32_t poll_interval_ms);
};
