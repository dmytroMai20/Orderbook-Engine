#include "MatchingEngine.h"
#include <thread>
#include <iostream>


MatchingEngine::MatchingEngine(
    std::vector<OrderRingBuffer*>& queues,
    Backpressure& backpressure,
    uint32_t burstSize)
    : queues_(queues),
      backpressure_(backpressure),
      burstSize_(burstSize)
{}

void MatchingEngine::start() {
    running_.store(true, std::memory_order_release);
    engineThread_ = std::thread(&MatchingEngine::run, this);
}

void MatchingEngine::stop() {
    running_.store(false, std::memory_order_release);
    if (engineThread_.joinable())
        engineThread_.join();
}

void MatchingEngine::print() const{
    const auto infos = orderbook_.GetOrderInfos();

    const auto& bids = infos.GetBids();
    const auto& asks = infos.GetAsks();

    std::cout << "\n================ ORDER BOOK ================\n";

    std::cout << "ASKS\n";
    std::cout << "Price\tQuantity\n";
    std::cout << "-------------------------\n";

    for (const auto& level : asks)
    {
        std::cout << level.price_ << "\t"
                  << level.quantity_ << "\n";
    }

    std::cout << "\nBIDS\n";
    std::cout << "Price\tQuantity\n";
    std::cout << "-------------------------\n";

    for (const auto& level : bids)
    {
        std::cout << level.price_ << "\t"
                  << level.quantity_ << "\n";
    }
    std::cout << "============================================\n";
    std::cout << "Event processed: " << eventsProcessed_ << "\n";
    std::cout << "Orderbook total ops: " << orderbook_.TotalOps() << "\n"; 
}

void MatchingEngine::run() {
    size_t index = 0;
    EngineEvent event;

    while (running_.load(std::memory_order_acquire)) {
        auto* queue = queues_[index];
        uint32_t processed = 0;

        while (processed < burstSize_ && queue->pop(event)) {
            processed++;
            backpressure_.decrement();

            switch (event.type) {
                case EngineEventType::Add:
                    orderbook_.AddOrder(
                        std::move(std::get<OrderPointer>(event.payload))
                    );
                    break;

                case EngineEventType::Cancel:
                    orderbook_.CancelOrder(
                        std::get<OrderId>(event.payload)
                    );
                    break;

                case EngineEventType::Modify:
                    orderbook_.ModifyOrder(
                        std::move(std::get<OrderModify>(event.payload))
                    );
                    break;

                case EngineEventType::Shutdown:
                    shutdownsReceived_++;
                    if (shutdownsReceived_ >= queues_.size())
                        running_.store(false, std::memory_order_release);
                    break;
            }
            eventsProcessed_++;
        }

        index = (index + 1) % queues_.size();

        if (processed == 0) {
            std::this_thread::yield();
        }
    }
}