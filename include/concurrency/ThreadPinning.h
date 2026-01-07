#pragma once

#include <cstdint>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#endif

inline bool PinCurrentThreadToCore(std::uint32_t coreIndex) noexcept
{
#if defined(__APPLE__)
    thread_affinity_policy_data_t policy{
        static_cast<integer_t>(coreIndex + 1)
    };

    const thread_port_t thread = pthread_mach_thread_np(pthread_self());
    const kern_return_t rc = thread_policy_set(
        thread,
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT);

    return rc == KERN_SUCCESS;
#else
    (void)coreIndex;
    return false;
#endif
}
