#include "Producer.h"

Producer::Producer(
    OrderRingBuffer& queue,
    std::atomic<bool>& running,
    uint32_t producer_id
)
    : queue_(queue)
    , running_(running)
    , producer_id_(producer_id)
    , rng_(producer_id) 
    , event_dist_(0, 2) // Add / Cancel / Modify
    , price_dist_(90, 110)
    , qty_dist_(1, 100)
    , side_dist_(0, 1)
{
}

void Producer::run() {
    while (running_.load(std::memory_order_relaxed)) {
        produce_event();
    }

    // Send shutdown event once
    while (!queue_.push(EngineEvent::MakeShutdown())) {
        /* backpressure */
    }
}

void Producer::produce_event() {
    const int event_type = event_dist_(rng_);
    Side side = side_dist_(rng_) == 0 ? Side::Buy : Side::Sell;
    switch (event_type) {
    case 0: { // Add
        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            OrderId{ rng_() },
            side,
            Price{ price_dist_(rng_) },
            Quantity{ qty_dist_(rng_) }
        );

        EngineEvent ev = EngineEvent::MakeAdd(std::move(order));
        while (!queue_.push(ev)) {
            /* backpressure */
        }
        break;
    }

    case 1: { // Cancel
        OrderId id{ rng_() };
        EngineEvent ev = EngineEvent::MakeCancel(id);
        while (!queue_.push(ev)) {
            /* backpressure */
        }
        break;
    }

    case 2: { // Modify
        OrderModify mod{
            OrderId{ rng_() },
            side_dist_(rng_) == 0 ? Side::Buy : Side::Sell,
            price_dist_(rng_),
            qty_dist_(rng_)
        };

        EngineEvent ev = EngineEvent::MakeModify(std::move(mod));
        while (!queue_.push(ev)) {
            /* backpressure */
        }
        break;
    }

    default:
        break;
    }
}
