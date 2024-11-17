cmake_minimum_required(VERSION 3.29)
project(utl)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(arena-allocator
        GIT_REPOSITORY https://github.com/RuslanUC/arena-allocator.git
        GIT_TAG 7a307225b5b3342af92767f37649653db49822ab)
FetchContent_MakeAvailable(arena-allocator)

add_library(utl STATIC
        src/utils.c
        src/map_list.c
        src/static_map.c
        src/string_view.c
        src/field_def.c
        src/type_def.c
        src/message_def.c
        src/def_pool.c
        src/message.c
        src/vector.c
        src/encoder.c
        src/decoder.c
        src/parser.c)
target_include_directories(utl PUBLIC include)
target_link_libraries(utl PUBLIC arena)

add_executable(utl_test main.cpp)
target_include_directories(utl_test PUBLIC include)
target_link_libraries(utl_test PUBLIC utl)
target_compile_options(utl_test PRIVATE -fpermissive -fsanitize=address)
target_link_options(utl_test PRIVATE -fsanitize=address)


include(FetchContent)
FetchContent_Declare(unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.0)
FetchContent_MakeAvailable(unity)

enable_testing()

add_executable(test_parser_exe tests/test_parser.c)
target_include_directories(test_parser_exe PRIVATE include)
target_link_libraries(test_parser_exe PRIVATE utl unity::framework)
target_compile_options(test_parser_exe PRIVATE -fpermissive -fsanitize=address)
target_link_options(test_parser_exe PRIVATE -fsanitize=address)
add_test(test_parser test_parser_exe)