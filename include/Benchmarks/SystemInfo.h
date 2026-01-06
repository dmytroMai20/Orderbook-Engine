#pragma once

#include <cstddef>
#include <cstdint>

namespace benchmarks {

struct SystemInfo {
    std::uint32_t cpuCores = 0;
    std::uint64_t availableMemoryBytes = 0;
    std::size_t pageSizeBytes = 0;
};

SystemInfo QuerySystemInfo() noexcept;

}
