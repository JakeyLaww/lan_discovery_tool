#include "core/PacketReceiveQueue.hpp"

PacketReceiveQueue::PacketReceiveQueue(size_t max_size) : max_size_(max_size) {}

void PacketReceiveQueue::push(std::vector<uint8_t> packet, std::string src_ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= max_size_) {
        queue_.pop_front();
        ++dropped_packets_;
    }
    queue_.emplace_back(std::move(packet), std::move(src_ip));
    cv_.notify_one();
}

bool PacketReceiveQueue::wait_pop(std::vector<uint8_t>& packet, std::string& src_ip,
                                  const std::function<bool()>& should_continue) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&] { return !queue_.empty() || !should_continue(); });
    if (!should_continue() && queue_.empty()) {
        return false;
    }
    packet = std::move(queue_.front().first);
    src_ip = std::move(queue_.front().second);
    queue_.pop_front();
    return true;
}

void PacketReceiveQueue::notify_all() {
    cv_.notify_all();
}

void PacketReceiveQueue::reset_dropped_count() {
    std::lock_guard<std::mutex> lock(mutex_);
    dropped_packets_ = 0;
}

size_t PacketReceiveQueue::dropped_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_packets_;
}
