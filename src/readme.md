# TODO
 - make this NUMA / cache-line aware
 - remove the EngineEventType enum entirely (variant-only dispatch)
 - test p50, p90, p99 and p99.9
 - implement prune good for day orders in matching engine rather than orderbook itself
 - refactor Scheduler and Backpressure
 - change file name from SPSCRingBuffer to queue
 - Change ringbuffer template to make it generic for any size
 - Core pinning