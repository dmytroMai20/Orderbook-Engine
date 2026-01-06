#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Percentiles.h"

namespace benchmarks {

struct RingBufferStats {
    std::size_t ringBufferSizeBytes = 0;
    std::size_t messageSizeBytes = 0;
    std::size_t messagesPerCacheLine = 0;
};

void PrintSetup(std::string_view benchName, const RingBufferStats& rbStats);
void PrintLatencyStats(std::string_view label, const LatencyPercentilesNs& pct);

}
