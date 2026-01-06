#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <iomanip>

#include "Producer.h"
#include "MatchingEngine.h"
#include "Backpressure.h"
#include "EngineEvent.h"
#include "OrderRingBuffer.h"

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::atomic<bool> running{true};

    constexpr std::size_t kNumProducers = 8;

    // One SPSC queue per producer
    std::vector<OrderRingBuffer> queues_storage(kNumProducers);
    std::vector<OrderRingBuffer*> queues;

    std::cout << "Allocating queues...\n";

    queues.reserve(kNumProducers);
    for (auto& q : queues_storage) {
        queues.push_back(&q);
    }

    for (auto& q : queues_storage) {
        q.prefault();
    }

    constexpr std::size_t kRingSize = 16384;
    constexpr std::size_t kRingCapacity = kRingSize - 1;
    Backpressure backpressure((kNumProducers * kRingCapacity * 3) / 4);

    MatchingEngine engine(
        queues,
        backpressure,
        64
    );

    std::cout << "Starting engine...\n";

    engine.start();

    std::cout << "Starting producers...\n";
    // Create producers
    std::vector<std::unique_ptr<Producer>> producers;
    std::vector<std::thread> producerThreads;

    producers.reserve(kNumProducers);
    producerThreads.reserve(kNumProducers);

    for (std::size_t i = 0; i < kNumProducers; ++i) {
        producers.emplace_back(std::make_unique<Producer>(*queues[i], backpressure, running, static_cast<uint32_t>(i)));
        producerThreads.emplace_back(&Producer::run, producers.back().get());
    }

    const auto start = std::chrono::steady_clock::now();

    std::atomic<bool> monitorRunning{true};
    std::thread monitor([&] {
        constexpr int kPeriodSeconds = 5;

        auto lastTime = std::chrono::steady_clock::now();
        std::uint64_t lastEvents = engine.EventsProcessed();
        std::uint64_t lastOps = engine.TotalOps();
        std::uint64_t lastIdle = engine.IdleLoops();
        std::uint64_t lastBpWaitCalls = backpressure.waitCalls.load(std::memory_order_relaxed);
        std::uint64_t lastBpWaitSpins = backpressure.waitSpins.load(std::memory_order_relaxed);

        std::vector<std::uint64_t> lastProd;
        std::vector<std::uint64_t> lastPool;
        std::vector<std::uint64_t> lastRetry;
        lastProd.resize(producers.size(), 0);
        lastPool.resize(producers.size(), 0);
        lastRetry.resize(producers.size(), 0);
        for (std::size_t i = 0; i < producers.size(); ++i) {
            lastProd[i] = producers[i]->ProducedEvents();
            lastPool[i] = producers[i]->PoolWaitSpins();
            lastRetry[i] = producers[i]->EnqueueRetries();
        }

        while (monitorRunning.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::seconds(kPeriodSeconds));

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<double> dt = now - lastTime;
            lastTime = now;

            const std::uint64_t events = engine.EventsProcessed();
            const std::uint64_t ops = engine.TotalOps();
            const std::uint64_t idle = engine.IdleLoops();

            const std::uint64_t dEvents = events - lastEvents;
            const std::uint64_t dOps = ops - lastOps;
            const std::uint64_t dIdle = idle - lastIdle;
            lastEvents = events;
            lastOps = ops;
            lastIdle = idle;

            std::uint64_t totalProd = 0;
            std::uint64_t totalPool = 0;
            std::uint64_t totalRetry = 0;
            for (std::size_t i = 0; i < producers.size(); ++i) {
                const auto p = producers[i]->ProducedEvents();
                const auto pw = producers[i]->PoolWaitSpins();
                const auto pr = producers[i]->EnqueueRetries();
                totalProd += (p - lastProd[i]);
                totalPool += (pw - lastPool[i]);
                totalRetry += (pr - lastRetry[i]);
                lastProd[i] = p;
                lastPool[i] = pw;
                lastRetry[i] = pr;
            }

            std::size_t qDepth = 0;
            for (auto* q : queues) {
                qDepth += q->size();
            }

            const auto bpWaitCalls = backpressure.waitCalls.load(std::memory_order_relaxed);
            const auto bpWaitSpins = backpressure.waitSpins.load(std::memory_order_relaxed);
            const auto dBpWaitCalls = bpWaitCalls - lastBpWaitCalls;
            const auto dBpWaitSpins = bpWaitSpins - lastBpWaitSpins;
            lastBpWaitCalls = bpWaitCalls;
            lastBpWaitSpins = bpWaitSpins;

            const double seconds = dt.count();
            std::cout
                << std::fixed << std::setprecision(2)
                << "[mon] ev/s=" << (seconds > 0.0 ? (static_cast<double>(dEvents) / seconds) : 0.0)
                << " ops/s=" << (seconds > 0.0 ? (static_cast<double>(dOps) / seconds) : 0.0)
                << " prod/s=" << (seconds > 0.0 ? (static_cast<double>(totalProd) / seconds) : 0.0)
                << " qDepth=" << qDepth
                << " orders=" << engine.OrderCount()
                << " idle=" << dIdle
                << " bpWaitCalls=" << dBpWaitCalls
                << " bpWaitSpins=" << dBpWaitSpins
                << " poolWaitSpins=" << totalPool
                << " enqueueRetries=" << totalRetry
                << "\n";
        }
    });

    std::cout << "Running for 20 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Stop producers
    running.store(false, std::memory_order_release);

    // Send shutdown event to each queue
    for (auto* q : queues) {
        uint32_t spins = 0;
        while (!q->push(EngineEvent::MakeShutdown())) {
            if (spins < 64) {
                ++spins;
                asm volatile("" ::: "memory");
            } else {
                spins = 0;
                std::this_thread::yield();
            }
        }
        backpressure.increment();
    }

    for (auto& t : producerThreads) {
        t.join();
    }

    monitorRunning.store(false, std::memory_order_relaxed);
    if (monitor.joinable())
        monitor.join();

    engine.stop();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    engine.print();

    const double seconds = elapsed.count();
    const double eventsPerSec = seconds > 0.0 ? (static_cast<double>(engine.EventsProcessed()) / seconds) : 0.0;
    const double opsPerSec = seconds > 0.0 ? (static_cast<double>(engine.TotalOps()) / seconds) : 0.0;
    std::cout << "Elapsed seconds: " << seconds << "\n";
    std::cout << "Events/sec: " << eventsPerSec << "\n";
    std::cout << "Orderbook ops/sec: " << opsPerSec << "\n";

    std::cout << "Shutdown complete.\n";
    return 0;
}
