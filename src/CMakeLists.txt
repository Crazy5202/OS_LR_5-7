cmake_minimum_required(VERSION 3.0)
project(ZMQ)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(control control.cpp)
add_executable(calc calc.cpp)

target_link_libraries(control zmq)
target_link_libraries(calc zmq)