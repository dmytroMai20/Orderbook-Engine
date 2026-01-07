#pragma once
#include <variant>
#include <cstdint>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"

struct ShutdownEvent {};

using EngineEventPayload =
    std::variant<OrderPointer, OrderId, OrderModify, ShutdownEvent>;

enum class EngineEventType : uint8_t {
    Add,
    Cancel,
    Modify,
    Shutdown
};

struct EngineEvent {
    EngineEventType type;
    EngineEventPayload payload;

    static EngineEvent MakeAdd(OrderPointer o) {
        return { EngineEventType::Add, std::move(o) };
    }

    static EngineEvent MakeCancel(OrderId id) {
        return { EngineEventType::Cancel, id };
    }

    static EngineEvent MakeModify(OrderModify m) {
        return { EngineEventType::Modify, std::move(m) };
    }

    static EngineEvent MakeShutdown() {
        return { EngineEventType::Shutdown, ShutdownEvent{} };
    }
};