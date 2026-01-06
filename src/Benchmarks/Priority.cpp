#include "Benchmarks/Priority.h"

#include <cerrno>

#if defined(__APPLE__)
#include <sys/resource.h>
#include <pthread.h>
#endif

namespace benchmarks {

PriorityResult RaiseProcessAndThreadPriorityBestEffort() noexcept {
    PriorityResult r;

#if defined(__APPLE__)
    errno = 0;
    const int prc = setpriority(PRIO_PROCESS, 0, -20);
    r.processPriorityRaised = (prc == 0);

    const int qos = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    r.threadPriorityRaised = (qos == 0);
#endif

    return r;
}

}
