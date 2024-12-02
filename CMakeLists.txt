cmake_minimum_required(VERSION 3.28)
project(utl)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(ext/arena-allocator)
add_subdirectory(utl)
add_subdirectory(python)
