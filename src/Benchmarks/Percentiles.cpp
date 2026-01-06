#include "Benchmarks/Percentiles.h"

#include <algorithm>
#include <cmath>

namespace benchmarks {

namespace {
inline std::uint64_t pick_percentile_sorted(const std::vector<std::uint64_t>& v, double p) {
    if (v.empty())
        return 0;

    const double x = p * static_cast<double>(v.size() - 1);
    std::size_t idx = static_cast<std::size_t>(std::llround(std::ceil(x)));
    if (idx >= v.size())
        idx = v.size() - 1;
    return v[idx];
}
}

LatencyPercentilesNs ComputeLatencyPercentilesNs(std::vector<std::uint64_t> samplesNs) {
    std::sort(samplesNs.begin(), samplesNs.end());

    LatencyPercentilesNs out;
    out.p50 = pick_percentile_sorted(samplesNs, 0.50);
    out.p95 = pick_percentile_sorted(samplesNs, 0.95);
    out.p99 = pick_percentile_sorted(samplesNs, 0.99);
    out.p999 = pick_percentile_sorted(samplesNs, 0.999);
    return out;
}

}
