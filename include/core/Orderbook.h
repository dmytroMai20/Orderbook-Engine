#pragma once

#include <map>
#include <unordered_map>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"
#include "OrderbookLevelInfos.h"
#include "Trade.h"
class Orderbook {
private:
    struct OrderEntry {
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    struct LevelData {
        Quantity quantity_{ };
        Quantity count_{ };
        enum class Action { Add, Remove, Match };
    };

    std::unordered_map<Price, LevelData> data_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    // 🔥 Operation counters (non-atomic, single-threaded engine)
    std::uint64_t addCount_{0};
    std::uint64_t cancelCount_{0};
    std::uint64_t modifyCount_{0};
    std::uint64_t executeCount_{0};

    void CancelOrderInternal(OrderId orderId);
    void OnOrderCancelled(OrderPointer order);
    void OnOrderAdded(OrderPointer order);
    void OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled);
    void UpdateLevelData(Price price, Quantity quantity, LevelData::Action action);
    bool CanFullyFill(Side side, Price price, Quantity quantity) const;
    bool CanMatch(Side side, Price price) const;
    Trades MatchOrders();

public:
    Orderbook() = default;
    Orderbook(const Orderbook&) = delete;
    void operator=(const Orderbook&) = delete;
    Orderbook(Orderbook&&) = delete;
    void operator=(Orderbook&&) = delete;
    ~Orderbook() = default;

    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId orderId);
    Trades ModifyOrder(OrderModify order);

    std::size_t Size() const;
    OrderbookLevelInfos GetOrderInfos() const;

    // 🔍 Optional accessors
    std::uint64_t TotalOps() const {
        return addCount_ + cancelCount_ + modifyCount_ + executeCount_;
    }
};