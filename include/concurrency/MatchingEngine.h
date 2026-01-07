#pragma once
#include <vector>
#include <thread>
#include <atomic>

#include "Orderbook.h"
#include "EngineEvent.h"
#include "SPSCRingBuffer.h"
#include "Backpressure.h"
#include "OrderRingBuffer.h"

class MatchingEngine {
public:
    MatchingEngine(
        std::vector<OrderRingBuffer*>& queues,
        Backpressure& backpressure,
        uint32_t burstSize = 64
    );

    void start();
    void stop();
    void print() const;

    uint32_t EventsProcessed() const { return eventsProcessed_; }
    std::uint64_t TotalOps() const { return orderbook_.TotalOps(); }
    std::size_t OrderCount() const { return orderbook_.Size(); }
    std::uint64_t IdleLoops() const { return idleLoops_.load(std::memory_order_relaxed); }

private:
    void run();

    Orderbook orderbook_;
    std::vector<OrderRingBuffer*> queues_;
    Backpressure& backpressure_;
    uint32_t burstSize_;
    uint32_t eventsProcessed_ = 0;
    uint32_t shutdownsReceived_ = 0;

    alignas(64) std::atomic<std::uint64_t> idleLoops_{0};

    std::atomic<bool> running_ = false;
    std::thread engineThread_ = std::thread();
};