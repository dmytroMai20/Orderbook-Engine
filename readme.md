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
- **Git**

> Dependencies (`fmt`, `GoogleTest`) are fetched automatically by CMake.

---

## Build

Out-of-source build is required.

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd Orderbook

cmake -S . -B build
cmake --build build