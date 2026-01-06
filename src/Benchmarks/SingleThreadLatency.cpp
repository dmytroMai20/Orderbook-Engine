#include "Benchmarks/BenchPrinter.h"
#include "Benchmarks/Percentiles.h"
#include "Benchmarks/Priority.h"

#include "EngineEvent.h"
#include "OrderRingBuffer.h"

#include <chrono>
#include <cstdint>
#include <iostream>
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

    return 0;
}
