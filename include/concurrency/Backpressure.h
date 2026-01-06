#pragma once
#include <atomic>
#include <thread>

struct Backpressure {
    std::atomic<size_t> count{0};
    size_t limit;

    std::atomic<std::uint64_t> waitCalls{0};
    std::atomic<std::uint64_t> waitSpins{0};

    explicit Backpressure(size_t l) : limit(l) {}

    void wait_if_needed() {
        waitCalls.fetch_add(1, std::memory_order_relaxed);
        uint32_t spins = 0;
        while (count.load(std::memory_order_acquire) >= limit) {
            waitSpins.fetch_add(1, std::memory_order_relaxed);

            if (spins < 128) {
                ++spins;
                asm volatile("" ::: "memory");
            } else {
                spins = 0;
                std::this_thread::yield();
            }
        }
    }

    void increment() { count.fetch_add(1, std::memory_order_acq_rel); }
    void decrement() { count.fetch_sub(1, std::memory_order_acq_rel); }
};