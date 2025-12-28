#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "Usings.h"
#include "OrderOp.h"
#include  "Side.h"

template<typename T, std::size_t Size>
class SPSCQueue {
    static_assert((Size & (Size - 1)) == 0,
                  "Size must be a power of two");

public:
    inline bool push(const T& item);
    inline bool pop(T& item);

    inline std::size_t size() const;
    inline bool empty() const;

private:
    alignas(64) std::atomic<std::size_t> head_{0};  // On macs the cache line is 128 bytes so use 128 to avoid false sharing. Maybe be interesting to look at perforamnce gain

    alignas(64) std::atomic<std::size_t> tail_{0};

    alignas(64) T buffer_[Size];
};

struct OrderEvent {
    OrderId  order_id;
    Price    price;
    Quantity quantity;
    uint64_t timestamp;
    OrderOp  op;
    Side     side;
}; // could combine OrderOp and Side into one uint8_t flag and use bit masking

extern SPSCQueue<OrderEvent, 65536> g_market_data_queue;