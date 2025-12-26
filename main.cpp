#include "Orderbook.h"
#include<iostream>

int main()
{
    Orderbook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout<< orderbook.Size() << std::endl;
    orderbook.CancelOrder(orderId);
    std::cout << orderbook.Size() << std::endl;
    // Do work.
    return 0;
}
