cmake_minimum_required(VERSION 3.29)
project(utl)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(arena-allocator
        GIT_REPOSITORY https://github.com/olemorud/arena-allocator.git
        GIT_TAG 786d7213ae2c505d8b378e41c8a87e83dac06761)
FetchContent_MakeAvailable(arena-allocator)

add_library(utl STATIC
        src/string_view.c
        src/field_def.c
        src/message_def.c
        src/message.c
        src/vector.c)
target_include_directories(utl PUBLIC include)
target_link_libraries(utl PUBLIC arena)

add_executable(utl_test main.cpp)
target_include_directories(utl_test PUBLIC include)
target_link_libraries(utl_test PUBLIC utl)
target_compile_options(utl_test PUBLIC "-fpermissive")
