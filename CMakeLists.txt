cmake_minimum_required(VERSION 3.20)
project(Orderbook LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)

# ---- Core library ----
add_library(orderbook_core
    src/core/Orderbook.cpp
    src/concurrency/MatchingEngine.cpp
    src/concurrency/Producer.cpp
)

target_include_directories(orderbook_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/concurrency
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(orderbook_core PUBLIC
    fmt::fmt
)

target_compile_features(orderbook_core PUBLIC cxx_std_20)

target_compile_options(orderbook_core PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-fno-exceptions>
    $<$<CONFIG:Release>:-fno-rtti>
    $<$<CONFIG:Release>:-fno-stack-protector>
    $<$<CONFIG:Release>:-finline-functions>
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Release>:-DNDEBUG>
)


# ---- Main executable ----
add_executable(Orderbook
    main.cpp
)

target_link_libraries(Orderbook PRIVATE
    orderbook_core
)

# ---- Benchmarks ----
add_executable(OrderbookBenchmarks
    src/Benchmarks/SingleThreadLatency.cpp
    src/Benchmarks/Priority.cpp
    src/Benchmarks/SystemInfo.cpp
    src/Benchmarks/Percentiles.cpp
    src/Benchmarks/BenchPrinter.cpp
)

target_compile_options(OrderbookBenchmarks PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
#    $<$<CONFIG:Release>:-fno-exceptions>
    $<$<CONFIG:Release>:-fno-rtti>
    $<$<CONFIG:Release>:-fno-stack-protector>
    $<$<CONFIG:Release>:-finline-functions>
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Release>:-DNDEBUG>
)

target_link_libraries(OrderbookBenchmarks PRIVATE
    orderbook_core
)

# ---- Tests ----
add_subdirectory(OrderbookTest)