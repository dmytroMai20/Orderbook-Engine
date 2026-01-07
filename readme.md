# Multi-order orderbook engine (C++)

Based on [Tzadiko's order book](https://github.com/Tzadiko/Orderbook)

A C++20 limit order book engine with clean separation between core logic, application code, and tests.  
Built using **CMake**, **fmt**, and **GoogleTest**.

> [!IMPORTANT] 
> Current implementation of benchmarks is only ported to macOS.

---
## Changes
 - Ported on CMake and changed dependecies to work on Apple/Linux machines
 - **Improved concurrency model:** N-producer, 1-consumer via N SPSC ring buffers (one per producer)
 - **Thread safety:** Lock-free SPSC queues and atomic backpressure; matching engine runs on a dedicated pinned thread
 - Added comprehensive single-thread latency benchmarks for queue and orderbook operations
 - Bug fixes
 - Restructured the project, separated into core library and tests

## Requirements

- **C++20 compiler**
  - macOS: Apple Clang ≥ 14
  - Linux: GCC ≥ 11 / Clang ≥ 14
  - Windows: MSVC (Visual Studio 2022)
- **CMake ≥ 3.20**

> Dependencies (`fmt`, `GoogleTest`) are fetched automatically by CMake.

---

## Benchmarks

A dedicated `OrderbookBenchmarks` executable provides single-thread latency measurements with percentile reporting (p50, p95, p99, p99.9) and system/ring-buffer configuration details.

### What is measured

- **SPSC queue round-trip latency:** `push` followed by `pop` on the lock-free `OrderRingBuffer` (size 16384)
- **Orderbook operation latency (single-thread):**
  - `Orderbook::AddOrder`
  - `Orderbook::CancelOrder`
  - `Orderbook::ModifyOrder`

All benchmarks report p50, p95, p99, and p99.9 percentiles in nanoseconds.

### Features

- **Best-effort priority boosting** (process and thread) on macOS
- **System info printing** (CPU cores, available memory, page size)
- **Ring-buffer stats** (size, message size, messages per cache line)
- **Preallocation to avoid allocation noise** in orderbook benchmarks

### Build and Run

```bash
# Build (Release recommended for realistic latency numbers)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run with default 1,000,000 iterations
./build/OrderbookBenchmarks

# Run with custom iteration count
./build/OrderbookBenchmarks 200000
```

#### Example Output on M2 Mac

```
Process priority raised: false
Thread priority raised: true

Single-thread latency benchmark
CPU cores available: 8
Available memory: 1157 MB
Ring buffer size: 655360 bytes (~0.625 MB, ~40 pages)
Message size: 40 bytes
Messages per cache line: 1
Page size: 16384 bytes

SPSC push+pop round-trip latency (ns): p50=42 p95=42 p99=42 p99.9=125
Orderbook AddOrder latency (ns): p50=83 p95=125 p99=167 p99.9=1625
Orderbook CancelOrder latency (ns): p50=125 p95=125 p99=209 p99.9=750
Orderbook ModifyOrder latency (ns): p50=250 p95=292 p99=375 p99.9=1834
```

### Implementation Details

- **Location**: `include/Benchmarks/` and `src/Benchmarks/`
- **Priority**: Uses `setpriority` and `pthread_set_qos_class_self_np` on macOS
- **Percentiles**: Simple sorting and nearest-rank method
- **Orderbook benchmarks**: Reuse preallocated `Order` objects to avoid allocation noise
- **Ring buffer**: Uses `OrderRingBuffer = SPSCQueue<EngineEvent, 16384>`
- **Concurrency model**: N producers → N SPSC ring buffers → 1 matching engine thread (round-robin drain)

---

## Concurrency Model

The engine uses an **N-producer, 1-consumer** design optimized for low-latency and cache-friendly operation:

- **Producers:** Each producer thread writes to its own lock-free SPSC ring buffer (`OrderRingBuffer`), avoiding contention.
- **Consumer:** The matching engine runs on a dedicated pinned thread, round-robin draining from all producer queues in configurable bursts.
- **Backpressure:** Atomic counter limits total in-flight events; producers spin/yield when the limit is reached.
- **Thread pinning:** On macOS, thread affinity is used to keep producer and engine threads on distinct cores.

This design eliminates mutexes and minimizes cache line sharing, enabling high-throughput order processing with deterministic latency.
