#pragma once
#include <atomic>
#include <thread>

struct Backpressure {
    std::atomic<size_t> count{0};
    size_t limit;

    explicit Backpressure(size_t l) : limit(l) {}

    void wait_if_needed() {
        while (count.load(std::memory_order_acquire) >= limit) {
            std::this_thread::yield();
        }
    }

    void increment() { count.fetch_add(1, std::memory_order_acq_rel); }
    void decrement() { count.fetch_sub(1, std::memory_order_acq_rel); }
};