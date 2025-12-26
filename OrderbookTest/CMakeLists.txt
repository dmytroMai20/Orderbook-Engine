add_executable(OrderbookTest
    test.cpp
    pch.cpp
)

target_link_libraries(OrderbookTest PRIVATE
    orderbook_core
    GTest::gtest
    GTest::gtest_main
)

target_compile_definitions(OrderbookTest
    PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/OrderbookTest/TestFiles"
)

target_include_directories(OrderbookTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)