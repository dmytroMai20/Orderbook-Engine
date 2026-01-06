# Multi-order orderbook engine (C++)

Based on [Tzadiko's order book](https://github.com/Tzadiko/Orderbook)

A C++20 limit order book engine with clean separation between core logic, application code, and tests.  
Built using **CMake**, **fmt**, and **GoogleTest**.

---
## Changes
 - Ported on CMake and changed dependecies to work on Apple/Linux machines
 - Improved concurrency model [TODO]
 - Thread-safety [TODO]
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

### Features

- **Best-effort priority boosting** (process and thread) on macOS
- **System info printing** (CPU cores, available memory, page size)
- **Ring-buffer stats** (size, message size, messages per cache line)
- **Percentile latency reporting** for:
  - SPSC queue push+pop round-trip
  - `Orderbook::AddOrder`
  - `Orderbook::CancelOrder`
  - `Orderbook::ModifyOrder`

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

#### Example Output

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

SPSC push+pop round-trip latency (ns): p50=41 p95=42 p99=42 p99.9=84
Orderbook AddOrder latency (ns): p50=416 p95=2708 p99=3333 p99.9=10708
Orderbook CancelOrder latency (ns): p50=83 p95=125 p99=167 p99.9=875
Orderbook ModifyOrder latency (ns): p50=1583 p95=5292 p99=8083 p99.9=20584
```

### Implementation Details

- **Location**: `include/Benchmarks/` and `src/Benchmarks/`
- **Priority**: Uses `setpriority` and `pthread_set_qos_class_self_np` on macOS
- **Percentiles**: Simple sorting and nearest-rank method
- **Orderbook benchmarks**: Reuse preallocated `Order` objects to avoid allocation noise
- **Ring buffer**: Uses `OrderRingBuffer = SPSCQueue<EngineEvent, 16384>`

---

## Build

Out-of-source build is required.

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd Orderbook

cmake -S . -B build
cmake --build build