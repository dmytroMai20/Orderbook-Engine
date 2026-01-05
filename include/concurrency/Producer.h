#pragma once

#include <atomic>
#include <cstdint>
#include <random>
#include <thread>

#include "EngineEvent.h"
#include "Usings.h"
#include "SPSCRingBuffer.h"
#include "Order.h"
#include "OrderModify.h"
#include "OrderRingBuffer.h"

class Producer {
public:
    Producer(
        OrderRingBuffer& queue,
        std::atomic<bool>& running,
        uint32_t producer_id
    );

    void run();

private:
    void produce_event();

private:
    OrderRingBuffer& queue_;
    std::atomic<bool>& running_;
    uint32_t producer_id_;

    // RNG
    std::mt19937 rng_;

    // Distributions
    std::uniform_int_distribution<int> event_dist_;   // Add / Cancel / Modify
    std::uniform_int_distribution<Price> price_dist_;
    std::uniform_int_distribution<Quantity> qty_dist_;
    std::uniform_int_distribution<int> side_dist_;
};