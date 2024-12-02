cmake_minimum_required(VERSION 3.28)
project(utl)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(ext/arena-allocator)
add_subdirectory(utl)
add_subdirectory(python)

add_executable(utl_test main.cpp)
target_include_directories(utl_test PUBLIC include)
target_link_libraries(utl_test PUBLIC utl)
target_compile_options(utl_test PRIVATE -fpermissive -fsanitize=address)
target_link_options(utl_test PRIVATE -fsanitize=address)
