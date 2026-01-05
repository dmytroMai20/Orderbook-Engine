#pragma once
#include <vector>
#include <cstddef>

class Scheduler {
public:
    Scheduler(size_t num_queues, size_t burst_size)
        : num_queues_(num_queues), burst_size_(burst_size) {}

    size_t next_queue(size_t& burst_counter, size_t& queue_idx) const {
        if (burst_counter == 0) {
            queue_idx = (queue_idx + 1) % num_queues_;
            burst_counter = burst_size_;
        }
        --burst_counter;
        return queue_idx;
    }

private:
    const size_t num_queues_;
    const size_t burst_size_;
};