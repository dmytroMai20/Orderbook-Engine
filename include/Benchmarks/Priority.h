#pragma once

#include <cstdint>

namespace benchmarks {

struct PriorityResult {
    bool processPriorityRaised = false;
    bool threadPriorityRaised = false;
};

PriorityResult RaiseProcessAndThreadPriorityBestEffort() noexcept;

}
