#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

template<typename T, std::size_t Size>
class SPSCQueue {
    static_assert((Size & (Size - 1)) == 0,
                  "Size must be a power of two");

public:
    SPSCQueue() = default;
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    inline bool push(const T& item) noexcept {
        const std::size_t current_tail =
            tail_.load(std::memory_order_relaxed);

        const std::size_t next_tail =
            (current_tail + 1) & (Size - 1);

        if (next_tail ==
            head_.load(std::memory_order_acquire)) {
            return false; // full
        }

        buffer_[current_tail] = item;

        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    inline bool push(T&& item) noexcept {
        const std::size_t current_tail =
            tail_.load(std::memory_order_relaxed);

        const std::size_t next_tail =
            (current_tail + 1) & (Size - 1);

        if (next_tail ==
            head_.load(std::memory_order_acquire)) {
            return false; // full
        }

        buffer_[current_tail] = std::move(item);

        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    inline bool pop(T& item) noexcept {
        const std::size_t current_head =
            head_.load(std::memory_order_relaxed);

        if (current_head ==
            tail_.load(std::memory_order_acquire)) {
            return false; // empty
        }

        item = buffer_[current_head];

        head_.store((current_head + 1) & (Size - 1),
                    std::memory_order_release);
        return true;
    }

    inline bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    inline std::size_t size() const noexcept {
        const std::size_t head =
            head_.load(std::memory_order_acquire);
        const std::size_t tail =
            tail_.load(std::memory_order_acquire);

        if (tail >= head) {
            return tail - head;
        }
        return Size + tail - head;
    }

private:
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

    alignas(64) T buffer_[Size];
};

