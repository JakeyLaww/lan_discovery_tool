#pragma once
#include "Logger.hpp"
#include "NetworkSocket.hpp"
#include "EventSink.hpp"
#include "core/MdnsPacketInterpreter.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

/**
 * @brief High-level discovery engine that broadcasts mDNS queries and listens for responses.
 */
class DiscoveryEngine {
public:
    static constexpr size_t kMaxPacketQueueSize = 256;

    explicit DiscoveryEngine(std::shared_ptr<Logger> logger);

    bool start();
    void broadcast_query();
    void set_event_sink(EventSinkPtr s);
    void set_verbose(bool verbose);
    void stop();
    bool is_running() const noexcept;
    bool run(uint32_t poll_interval_ms = 5000, std::function<bool()> shutdown_requested = nullptr);

private:
    std::unique_ptr<NetworkSocket> socket;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<MdnsPacketInterpreter> packet_interpreter;
    std::atomic<bool> running{false};
    std::atomic<bool> error_occurred_{false};
    EventSinkPtr sink;

    std::thread receiver_thread;
    std::thread worker_thread;
    std::thread poller_thread;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<std::pair<std::vector<uint8_t>, std::string>> packet_queue;
    size_t dropped_packets_{0};

    void receiver_loop();
    void worker_loop();
    void poller_loop(uint32_t poll_interval_ms);
};
