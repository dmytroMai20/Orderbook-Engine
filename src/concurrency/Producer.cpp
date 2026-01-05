#include "Producer.h"

Producer::Producer(
    OrderRingBuffer& queue,
    Backpressure& backpressure,
    std::atomic<bool>& running,
    uint32_t producer_id
)
    : queue_(queue)
    , backpressure_(backpressure)
    , running_(running)
    , producer_id_(producer_id)
    , rng_state_(producer_id ? producer_id : 1u)
{
}

uint32_t Producer::next_u32() noexcept {
    uint32_t x = rng_state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state_ = x;
    return x;
}

void Producer::run() {
    while (running_.load(std::memory_order_relaxed)) {
        produce_event();
    }
}

void Producer::produce_event() {
    const int event_type = static_cast<int>(next_u32() % 3u);
    const Side side = ((next_u32() & 1u) == 0u) ? Side::Buy : Side::Sell;
    switch (event_type) {
    case 0: { // Add
        const Price price = static_cast<Price>(90 + (next_u32() % 21u));
        const Quantity qty = static_cast<Quantity>(1 + (next_u32() % 100u));
        const OrderId id = (static_cast<OrderId>(producer_id_) << 32) | static_cast<OrderId>(order_seq_++);

        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            id,
            side,
            price,
            qty
        );

        EngineEvent ev = EngineEvent::MakeAdd(std::move(order));
        backpressure_.wait_if_needed();
        while (!queue_.push(std::move(ev))) {
            std::this_thread::yield();
        }
        backpressure_.increment();
        break;
    }

    case 1: { // Cancel
        OrderId id{ next_u32() };
        EngineEvent ev = EngineEvent::MakeCancel(id);
        backpressure_.wait_if_needed();
        while (!queue_.push(std::move(ev))) {
            std::this_thread::yield();
        }
        backpressure_.increment();
        break;
    }

    case 2: { // Modify
        OrderModify mod{
            OrderId{ next_u32() },
            ((next_u32() & 1u) == 0u) ? Side::Buy : Side::Sell,
            static_cast<Price>(90 + (next_u32() % 21u)),
            static_cast<Quantity>(1 + (next_u32() % 100u))
        };

        EngineEvent ev = EngineEvent::MakeModify(std::move(mod));
        backpressure_.wait_if_needed();
        while (!queue_.push(std::move(ev))) {
            std::this_thread::yield();
        }
        backpressure_.increment();
        break;
    }

    default:
        break;
    }
}
