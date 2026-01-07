#pragma once

#include <atomic>
#include <cstdint>
#include <array>
#include <memory>
#include <type_traits>
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

    std::uint64_t ProducedEvents() const { return producedEvents_.load(std::memory_order_relaxed); }
    std::uint64_t PoolWaitSpins() const { return poolWaitSpins_.load(std::memory_order_relaxed); }
    std::uint64_t EnqueueRetries() const { return enqueueRetries_.load(std::memory_order_relaxed); }

private:
    void produce_event();
    uint32_t next_u32() noexcept;

    struct OrderPool {
        static constexpr std::size_t FreelistSize = 1u << 16;
        static constexpr std::size_t PoolSize = FreelistSize - 1;
        using Storage = std::aligned_storage_t<sizeof(Order), alignof(Order)>;

        OrderPool();
        ~OrderPool();

        Order* acquire() noexcept;
        void release(Order* p) noexcept;

        std::unique_ptr<Storage[]> storage_;
        SPSCQueue<Order*, FreelistSize> freelist_;
    };

private:
    OrderRingBuffer& queue_;
    Backpressure& backpressure_;
    std::atomic<bool>& running_;
    uint32_t producer_id_;

    uint32_t rng_state_;
    uint64_t order_seq_ = 0;

    std::shared_ptr<OrderPool> pool_;

    static constexpr std::size_t IdRingSize = 1u << 12;
    std::array<OrderId, IdRingSize> id_ring_{};
    std::size_t id_ring_pos_ = 0;
    std::size_t id_ring_count_ = 0;

    alignas(64) std::atomic<std::uint64_t> producedEvents_{0};
    alignas(64) std::atomic<std::uint64_t> poolWaitSpins_{0};
    alignas(64) std::atomic<std::uint64_t> enqueueRetries_{0};
};