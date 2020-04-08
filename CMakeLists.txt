cmake_minimum_required(VERSION 3.6)
project(orderbook)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(order_book_shared SHARED orderbook.cpp)

add_executable(order_book orderbook_main.cpp)
target_link_libraries(order_book order_book_shared)

add_executable(order_book_test orderbook_test.cpp)
target_link_libraries(order_book_test order_book_shared)
