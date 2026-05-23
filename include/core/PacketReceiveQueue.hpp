#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Thread-safe bounded queue of received UDP payloads and sender IPs.
 */
class PacketReceiveQueue {
public:
    explicit PacketReceiveQueue(size_t max_size);

    void push(std::vector<uint8_t> packet, std::string src_ip);

    /**
     * @brief Block until a packet is available or should_continue returns false.
     * @return false if the queue is empty and scanning has stopped.
     */
    bool wait_pop(std::vector<uint8_t>& packet, std::string& src_ip,
                  const std::function<bool()>& should_continue);

    void notify_all();

    void reset_dropped_count();

    size_t dropped_count() const;

private:
    size_t max_size_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::pair<std::vector<uint8_t>, std::string>> queue_;
    size_t dropped_packets_{0};
};
