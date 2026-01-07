#pragma once

#include <cstdint>
#include <vector>

namespace benchmarks {

struct LatencyPercentilesNs {
    std::uint64_t p50 = 0;
    std::uint64_t p95 = 0;
    std::uint64_t p99 = 0;
    std::uint64_t p999 = 0;
};

LatencyPercentilesNs ComputeLatencyPercentilesNs(std::vector<std::uint64_t> samplesNs);

}
