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
    core/Orderbook.cpp
)

target_include_directories(orderbook_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(orderbook_core PUBLIC
    fmt::fmt
)

# ---- Main executable ----
add_executable(Orderbook
    main.cpp
)

target_link_libraries(Orderbook PRIVATE
    orderbook_core
)

# ---- Tests ----
add_subdirectory(OrderbookTest)