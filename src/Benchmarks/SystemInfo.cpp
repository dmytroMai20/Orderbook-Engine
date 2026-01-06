#include "Benchmarks/SystemInfo.h"

#include <thread>

#if defined(__APPLE__)
#include <unistd.h>
#include <mach/mach.h>
#endif

namespace benchmarks {

SystemInfo QuerySystemInfo() noexcept {
    SystemInfo info;

    const unsigned hc = std::thread::hardware_concurrency();
    info.cpuCores = hc ? static_cast<std::uint32_t>(hc) : 1u;

#if defined(__APPLE__)
    const long ps = sysconf(_SC_PAGESIZE);
    info.pageSizeBytes = ps > 0 ? static_cast<std::size_t>(ps) : static_cast<std::size_t>(4096);

    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmStats{};
    const kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmStats), &count);
    if (kr == KERN_SUCCESS) {
        const std::uint64_t freeBytes = static_cast<std::uint64_t>(vmStats.free_count) * static_cast<std::uint64_t>(info.pageSizeBytes);
        const std::uint64_t inactiveBytes = static_cast<std::uint64_t>(vmStats.inactive_count) * static_cast<std::uint64_t>(info.pageSizeBytes);
        info.availableMemoryBytes = freeBytes + inactiveBytes;
    }
#else
    info.pageSizeBytes = 4096;
    info.availableMemoryBytes = 0;
#endif

    return info;
}

}
