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
    std::atomic<bool> running{true};

    constexpr std::size_t kNumProducers = 1;

    // One SPSC queue per producer
    std::vector<OrderRingBuffer> queues_storage(kNumProducers);
    std::vector<OrderRingBuffer*> queues;

    queues.reserve(kNumProducers);
    for (auto& q : queues_storage) {
        queues.push_back(&q);
    }

    Backpressure backpressure(size_t{100});

    MatchingEngine engine(
        queues,
        backpressure,
        64
    );

    engine.start();

    // Create producers
    std::vector<Producer> producers;
    std::vector<std::thread> producerThreads;

    producers.reserve(kNumProducers);
    producerThreads.reserve(kNumProducers);

    for (std::size_t i = 0; i < kNumProducers; ++i) {
        producers.emplace_back(*queues[i], running, static_cast<uint32_t>(i));
        producerThreads.emplace_back(&Producer::run, &producers.back());
    }

    std::cout << "Running for 1 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop producers
    running.store(false, std::memory_order_release);

    // Send shutdown event to each queue
    for (auto* q : queues) {
        while (!q->push(EngineEvent::MakeShutdown())) {
            std::this_thread::yield();
        }
    }

    for (auto& t : producerThreads) {
        t.join();
    }

    engine.stop();
    engine.print();

    std::cout << "Shutdown complete.\n";
    return 0;
}
