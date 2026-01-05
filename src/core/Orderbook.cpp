#include "Orderbook.h"

#include <numeric>

void Orderbook::CancelOrderInternal(OrderId orderId)
{
	++cancelCount_;

    if (!orders_.contains(orderId))
        return;

    const auto [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell)
    {
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            bids_.erase(price);
    }

    OnOrderCancelled(order);
}

void Orderbook::OnOrderCancelled(OrderPointer order)
{
    UpdateLevelData(order->GetPrice(),
                    order->GetRemainingQuantity(),
                    LevelData::Action::Remove);
}

void Orderbook::OnOrderAdded(OrderPointer order)
{
    UpdateLevelData(order->GetPrice(),
                    order->GetInitialQuantity(),
                    LevelData::Action::Add);
}

void Orderbook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled)
{
    UpdateLevelData(price,
                    quantity,
                    isFullyFilled ? LevelData::Action::Remove
                                  : LevelData::Action::Match);
}

void Orderbook::UpdateLevelData(Price price, Quantity quantity, LevelData::Action action)
{
    auto& data = data_[price];

    data.count_ += (action == LevelData::Action::Add)
                     ? 1
                     : (action == LevelData::Action::Remove ? -1 : 0);

    if (action == LevelData::Action::Remove || action == LevelData::Action::Match)
        data.quantity_ -= quantity;
    else
        data.quantity_ += quantity;

    if (data.count_ == 0)
        data_.erase(price);
}

bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
    if (!CanMatch(side, price))
        return false;

    std::optional<Price> threshold;

    if (side == Side::Buy)
        threshold = asks_.begin()->first;
    else
        threshold = bids_.begin()->first;

    for (const auto& [levelPrice, levelData] : data_)
    {
        if ((side == Side::Buy && levelPrice > price) ||
            (side == Side::Sell && levelPrice < price))
            continue;

        if (quantity <= levelData.quantity_)
            return true;

        quantity -= levelData.quantity_;
    }

    return false;
}

bool Orderbook::CanMatch(Side side, Price price) const
{
    if (side == Side::Buy)
        return !asks_.empty() && price >= asks_.begin()->first;
    else
        return !bids_.empty() && price <= bids_.begin()->first;
}

Trades Orderbook::MatchOrders()
{
    Trades trades;
    trades.reserve(orders_.size());

    while (!bids_.empty() && !asks_.empty())
    {
        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice)
            break;

        while (!bids.empty() && !asks.empty())
        {
            auto bid = bids.front();
            auto ask = asks.front();

            Quantity quantity =
                std::min(bid->GetRemainingQuantity(),
                         ask->GetRemainingQuantity());

            bid->Fill(quantity);
            ask->Fill(quantity);

			++executeCount_;

            if (bid->IsFilled())
            {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }

            if (ask->IsFilled())
            {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }

            trades.push_back(Trade{
                { bid->GetOrderId(), bid->GetPrice(), quantity },
                { ask->GetOrderId(), ask->GetPrice(), quantity }
            });

            OnOrderMatched(bid->GetPrice(), quantity, bid->IsFilled());
            OnOrderMatched(ask->GetPrice(), quantity, ask->IsFilled());
        }

        if (bids.empty())
        {
            bids_.erase(bidPrice);
            data_.erase(bidPrice);
        }

        if (asks.empty())
        {
            asks_.erase(askPrice);
            data_.erase(askPrice);
        }
    }

    return trades;
}

Trades Orderbook::AddOrder(OrderPointer order)
{
	++addCount_;

    if (orders_.contains(order->GetOrderId()))
        return { };

    if (order->GetOrderType() == OrderType::Market)
    {
        if (order->GetSide() == Side::Buy && !asks_.empty())
            order->ToGoodTillCancel(asks_.rbegin()->first);
        else if (order->GetSide() == Side::Sell && !bids_.empty())
            order->ToGoodTillCancel(bids_.rbegin()->first);
        else
            return { };
    }

    if (order->GetOrderType() == OrderType::FillAndKill &&
        !CanMatch(order->GetSide(), order->GetPrice()))
        return { };

    if (order->GetOrderType() == OrderType::FillOrKill &&
        !CanFullyFill(order->GetSide(),
                      order->GetPrice(),
                      order->GetInitialQuantity()))
        return { };

    OrderPointers::iterator iterator;

    if (order->GetSide() == Side::Buy)
    {
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    {
        auto& orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({ order->GetOrderId(), { order, iterator } });
    OnOrderAdded(order);

    return MatchOrders();
}

void Orderbook::CancelOrder(OrderId orderId)
{
    CancelOrderInternal(orderId);
}

Trades Orderbook::ModifyOrder(OrderModify order)
{
	++modifyCount_;

    if (!orders_.contains(order.GetOrderId()))
        return { };

    auto orderType = orders_.at(order.GetOrderId()).order_->GetOrderType();
    CancelOrder(order.GetOrderId());
    return AddOrder(order.ToOrderPointer(orderType));
}

std::size_t Orderbook::Size() const
{
    return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(bids_.size());
    askInfos.reserve(asks_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
    {
        return LevelInfo{
            price,
            std::accumulate(
                orders.begin(),
                orders.end(),
                Quantity{0},
                [](Quantity sum, const OrderPointer& o)
                { return sum + o->GetRemainingQuantity(); })
        };
    };

    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));

    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    return { bidInfos, askInfos };
}