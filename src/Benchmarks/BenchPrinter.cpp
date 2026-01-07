#include "Benchmarks/BenchPrinter.h"

#include "Benchmarks/SystemInfo.h"

#include <iostream>

namespace benchmarks {

namespace {
inline std::uint64_t bytes_to_mb(std::uint64_t bytes) {
    return bytes / (1024ull * 1024ull);
}
}

void PrintSetup(std::string_view benchName, const RingBufferStats& rbStats) {
    const SystemInfo sys = QuerySystemInfo();

    std::cout << benchName << "\n";
    std::cout << "CPU cores available: " << sys.cpuCores << "\n";
    std::cout << "Available memory: " << bytes_to_mb(sys.availableMemoryBytes) << " MB\n";

    const double mb = static_cast<double>(rbStats.ringBufferSizeBytes) / (1024.0 * 1024.0);
    const std::uint64_t pages = sys.pageSizeBytes ? (rbStats.ringBufferSizeBytes / sys.pageSizeBytes) : 0;

    std::cout << "Ring buffer size: " << rbStats.ringBufferSizeBytes << " bytes (~" << mb << " MB, ~" << pages << " pages)\n";
    std::cout << "Message size: " << rbStats.messageSizeBytes << " bytes\n";
    std::cout << "Messages per cache line: " << rbStats.messagesPerCacheLine << "\n";
    std::cout << "Page size: " << sys.pageSizeBytes << " bytes\n";
    std::cout << "\n";
}

void PrintLatencyStats(std::string_view label, const LatencyPercentilesNs& pct) {
    std::cout << label << " latency (ns): "
              << "p50=" << pct.p50
              << " p95=" << pct.p95
              << " p99=" << pct.p99
              << " p99.9=" << pct.p999
              << "\n";
}

}
