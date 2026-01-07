#include "Benchmarks/BenchPrinter.h"
#include "Benchmarks/Percentiles.h"
#include "Benchmarks/Priority.h"

#include "EngineEvent.h"
#include "OrderRingBuffer.h"
#include "Orderbook.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace {

benchmarks::RingBufferStats MakeOrderRingBufferStats() {
    benchmarks::RingBufferStats s;
    s.messageSizeBytes = sizeof(EngineEvent);
    s.ringBufferSizeBytes = sizeof(EngineEvent) * static_cast<std::size_t>(16384);
    constexpr std::size_t kCacheLine = 64;
    s.messagesPerCacheLine = s.messageSizeBytes ? (kCacheLine / s.messageSizeBytes) : 0;
    if (s.messagesPerCacheLine == 0)
        s.messagesPerCacheLine = 1;
    return s;
}

benchmarks::LatencyPercentilesNs RunSingleThreadQueueRoundTripLatency(std::size_t iterations) {
    OrderRingBuffer q;
    q.prefault();

    std::vector<std::uint64_t> samples;
    samples.reserve(iterations);

    EngineEvent ev = EngineEvent::MakeCancel(OrderId{1});
    EngineEvent out;

    for (std::size_t i = 0; i < 10000; ++i) {
        while (!q.push(ev)) {
        }
        while (!q.pop(out)) {
        }
    }

    for (std::size_t i = 0; i < iterations; ++i) {
        ev = EngineEvent::MakeCancel(static_cast<OrderId>(i));

        const auto t0 = std::chrono::steady_clock::now();
        while (!q.push(ev)) {
        }
        while (!q.pop(out)) {
        }
        const auto t1 = std::chrono::steady_clock::now();

        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        samples.push_back(static_cast<std::uint64_t>(ns));
    }

    return benchmarks::ComputeLatencyPercentilesNs(std::move(samples));
}

benchmarks::LatencyPercentilesNs RunSingleThreadOrderbookAddLatency(std::size_t iterations) {
    Orderbook ob;

    std::vector<Order> storage;
    storage.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        storage.emplace_back(OrderType::GoodTillCancel,
                             OrderId{i + 1},
                             Side::Buy,
                             Price{100},
                             Quantity{1});
    }

    std::vector<OrderPointer> orders;
    orders.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        orders.emplace_back(OrderPointer(&storage[i], [](Order*) {}));
    }

    std::vector<std::uint64_t> samples;
    samples.reserve(iterations);

    for (std::size_t i = 0; i < 1000 && i < iterations; ++i) {
        (void)ob.AddOrder(orders[i]);
    }

    for (std::size_t i = 0; i < iterations; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        (void)ob.AddOrder(orders[i]);
        const auto t1 = std::chrono::steady_clock::now();

        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        samples.push_back(static_cast<std::uint64_t>(ns));
    }

    return benchmarks::ComputeLatencyPercentilesNs(std::move(samples));
}

benchmarks::LatencyPercentilesNs RunSingleThreadOrderbookCancelLatency(std::size_t iterations) {
    Orderbook ob;

    std::vector<Order> storage;
    storage.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        storage.emplace_back(OrderType::GoodTillCancel,
                             OrderId{i + 1},
                             Side::Buy,
                             Price{100},
                             Quantity{1});
    }

    std::vector<OrderPointer> orders;
    orders.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        orders.emplace_back(OrderPointer(&storage[i], [](Order*) {}));
    }

    for (std::size_t i = 0; i < iterations; ++i) {
        (void)ob.AddOrder(orders[i]);
    }

    std::vector<std::uint64_t> samples;
    samples.reserve(iterations);

    for (std::size_t i = 0; i < iterations; ++i) {
        const OrderId id = OrderId{i + 1};
        const auto t0 = std::chrono::steady_clock::now();
        ob.CancelOrder(id);
        const auto t1 = std::chrono::steady_clock::now();

        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        samples.push_back(static_cast<std::uint64_t>(ns));
    }

    return benchmarks::ComputeLatencyPercentilesNs(std::move(samples));
}

benchmarks::LatencyPercentilesNs RunSingleThreadOrderbookModifyLatency(std::size_t iterations) {
    Orderbook ob;

    std::vector<Order> storage;
    storage.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        storage.emplace_back(OrderType::GoodTillCancel,
                             OrderId{i + 1},
                             Side::Buy,
                             Price{100},
                             Quantity{1});
    }

    std::vector<OrderPointer> orders;
    orders.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
        orders.emplace_back(OrderPointer(&storage[i], [](Order*) {}));
    }

    for (std::size_t i = 0; i < iterations; ++i) {
        (void)ob.AddOrder(orders[i]);
    }

    std::vector<std::uint64_t> samples;
    samples.reserve(iterations);

    for (std::size_t i = 0; i < iterations; ++i) {
        const OrderId id = OrderId{i + 1};
        OrderModify mod{id, Side::Buy, Price{101}, Quantity{1}};

        const auto t0 = std::chrono::steady_clock::now();
        (void)ob.ModifyOrder(mod);
        const auto t1 = std::chrono::steady_clock::now();

        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        samples.push_back(static_cast<std::uint64_t>(ns));
    }

    return benchmarks::ComputeLatencyPercentilesNs(std::move(samples));
}

}

int main(int argc, char** argv) {
    std::size_t iterations = 1'000'000;
    if (argc >= 2) {
        try {
            iterations = static_cast<std::size_t>(std::stoull(argv[1]));
        } catch (...) {
        }
    }

    const auto pr = benchmarks::RaiseProcessAndThreadPriorityBestEffort();
    std::cout << "Process priority raised: " << (pr.processPriorityRaised ? "true" : "false") << "\n";
    std::cout << "Thread priority raised: " << (pr.threadPriorityRaised ? "true" : "false") << "\n\n";

    const auto stats = MakeOrderRingBufferStats();
    benchmarks::PrintSetup("Single-thread latency benchmark", stats);

    const auto pct = RunSingleThreadQueueRoundTripLatency(iterations);
    benchmarks::PrintLatencyStats("SPSC push+pop round-trip", pct);

    const auto addPct = RunSingleThreadOrderbookAddLatency(iterations);
    benchmarks::PrintLatencyStats("Orderbook AddOrder", addPct);

    const auto cancelPct = RunSingleThreadOrderbookCancelLatency(iterations);
    benchmarks::PrintLatencyStats("Orderbook CancelOrder", cancelPct);

    const auto modifyPct = RunSingleThreadOrderbookModifyLatency(iterations);
    benchmarks::PrintLatencyStats("Orderbook ModifyOrder", modifyPct);

    return 0;
}
