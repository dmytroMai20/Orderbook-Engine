#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

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

    Backpressure backpressure(kNumProducers * 1024);

    MatchingEngine engine(
        queues,
        backpressure,
        64
    );

    std::cout << "Starting engine...\n";

    engine.start();

    std::cout << "Starting producers...\n";
    // Create producers
    std::vector<Producer> producers;
    std::vector<std::thread> producerThreads;

    producers.reserve(kNumProducers);
    producerThreads.reserve(kNumProducers);

    for (std::size_t i = 0; i < kNumProducers; ++i) {
        producers.emplace_back(*queues[i], backpressure, running, static_cast<uint32_t>(i));
        producerThreads.emplace_back(&Producer::run, &producers.back());
    }

    const auto start = std::chrono::steady_clock::now();

    std::cout << "Running for 1 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(10));

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
