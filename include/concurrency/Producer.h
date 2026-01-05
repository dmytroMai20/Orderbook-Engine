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
#include "Backpressure.h"

class Producer {
public:
    Producer(
        OrderRingBuffer& queue,
        Backpressure& backpressure,
        std::atomic<bool>& running,
        uint32_t producer_id
    );

    void run();

private:
    void produce_event();
    uint32_t next_u32() noexcept;

private:
    OrderRingBuffer& queue_;
    Backpressure& backpressure_;
    std::atomic<bool>& running_;
    uint32_t producer_id_;

    uint32_t rng_state_;
    uint64_t order_seq_ = 0;
};