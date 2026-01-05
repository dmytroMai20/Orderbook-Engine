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

private:
    void run();

    Orderbook orderbook_;
    std::vector<OrderRingBuffer*> queues_;
    Backpressure& backpressure_;
    uint32_t burstSize_;
    uint32_t eventsProcessed_;

    std::atomic<bool> running_{false};
    std::thread engineThread_;
};