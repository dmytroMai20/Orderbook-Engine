#include "Producer.h"
#include "ThreadPinning.h"

namespace {
inline void backoff(uint32_t& spins) noexcept {
    if (spins < 64) {
        ++spins;
        asm volatile("" ::: "memory");
        return;
    }
    spins = 0;
    std::this_thread::yield();
}
}

Producer::OrderPool::OrderPool()
    : storage_(std::make_unique<Storage[]>(PoolSize))
{
    for (std::size_t i = 0; i < PoolSize; ++i)
    {
        auto* p = reinterpret_cast<Order*>(&storage_[i]);
        new (p) Order(OrderType::GoodTillCancel, OrderId{0}, Side::Buy, Price{0}, Quantity{0});
        while (!freelist_.push(p)) {
        }
    }
}

Producer::OrderPool::~OrderPool()
{
    for (std::size_t i = 0; i < PoolSize; ++i)
    {
        auto* p = reinterpret_cast<Order*>(&storage_[i]);
        p->~Order();
    }
}

Order* Producer::OrderPool::acquire() noexcept
{
    Order* p = nullptr;
    if (freelist_.pop(p))
        return p;
    return nullptr;
}

void Producer::OrderPool::release(Order* p) noexcept
{
    if (!p)
        return;
    uint32_t spins = 0;
    while (!freelist_.push(p)) {
        backoff(spins);
    }
}

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
    , pool_(std::make_shared<OrderPool>())
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
    PinCurrentThreadToCore(producer_id_ + 1);

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

        id_ring_[id_ring_pos_] = id;
        id_ring_pos_ = (id_ring_pos_ + 1) & (IdRingSize - 1);
        if (id_ring_count_ < IdRingSize)
            ++id_ring_count_;

        Order* raw = nullptr;
        bool fromPool = true;
        uint32_t allocSpins = 0;
        uint32_t allocFails = 0;
        while ((raw = pool_->acquire()) == nullptr) {
            if (!running_.load(std::memory_order_relaxed))
                return;
            poolWaitSpins_.fetch_add(1, std::memory_order_relaxed);
            backoff(allocSpins);

            if (++allocFails >= 1024) {
                fromPool = false;
                raw = new Order(OrderType::GoodTillCancel, OrderId{0}, Side::Buy, Price{0}, Quantity{0});
                break;
            }
        }

        raw->Reset(OrderType::GoodTillCancel, id, side, price, qty);

        auto order = fromPool
            ? OrderPointer(raw, [pool = pool_](Order* p) { pool->release(p); })
            : OrderPointer(raw, [](Order* p) { delete p; });

        EngineEvent ev = EngineEvent::MakeAdd(std::move(order));
        backpressure_.wait_if_needed();
        uint32_t spins = 0;
        while (!queue_.push(ev)) {
            enqueueRetries_.fetch_add(1, std::memory_order_relaxed);
            backpressure_.wait_if_needed();
            backoff(spins);
        }
        backpressure_.increment();
        producedEvents_.fetch_add(1, std::memory_order_relaxed);
        break;
    }

    case 1: { // Cancel
        OrderId id{ next_u32() };
        if (id_ring_count_ > 0)
        {
            const std::size_t offset = static_cast<std::size_t>(next_u32() % id_ring_count_);
            const std::size_t idx = (id_ring_pos_ - 1 - offset) & (IdRingSize - 1);
            id = id_ring_[idx];
        }
        EngineEvent ev = EngineEvent::MakeCancel(id);
        backpressure_.wait_if_needed();
        uint32_t spins = 0;
        while (!queue_.push(ev)) {
            enqueueRetries_.fetch_add(1, std::memory_order_relaxed);
            backpressure_.wait_if_needed();
            backoff(spins);
        }
        backpressure_.increment();
        producedEvents_.fetch_add(1, std::memory_order_relaxed);
        break;
    }

    case 2: { // Modify
        OrderId id{ next_u32() };
        if (id_ring_count_ > 0)
        {
            const std::size_t offset = static_cast<std::size_t>(next_u32() % id_ring_count_);
            const std::size_t idx = (id_ring_pos_ - 1 - offset) & (IdRingSize - 1);
            id = id_ring_[idx];
        }
        OrderModify mod{
            id,
            ((next_u32() & 1u) == 0u) ? Side::Buy : Side::Sell,
            static_cast<Price>(90 + (next_u32() % 21u)),
            static_cast<Quantity>(1 + (next_u32() % 100u))
        };

        EngineEvent ev = EngineEvent::MakeModify(std::move(mod));
        backpressure_.wait_if_needed();
        uint32_t spins = 0;
        while (!queue_.push(ev)) {
            enqueueRetries_.fetch_add(1, std::memory_order_relaxed);
            backpressure_.wait_if_needed();
            backoff(spins);
        }
        backpressure_.increment();
        producedEvents_.fetch_add(1, std::memory_order_relaxed);
        break;
    }

    default:
        break;
    }
}
