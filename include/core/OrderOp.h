#pragma once

#include <cstdint>

enum class OrderOp : std::uint8_t {
    Add,
    Cancel,
    Modify
};