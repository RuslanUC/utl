set(CMAKE_CXX_STANDARD 20)

#target_compile_options(arena PRIVATE "-fPIC")

add_library(utl STATIC
        src/arena.c
        src/status.c
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
#target_link_libraries(utl PUBLIC arena)
target_compile_options(utl PUBLIC "-fPIC")

include(FetchContent)
FetchContent_Declare(unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.0)
FetchContent_MakeAvailable(unity)

enable_testing()

target_compile_definitions(unity PRIVATE UNITY_INCLUDE_DOUBLE)

add_executable(test_parser_exe tests/test_parser.c)
target_include_directories(test_parser_exe PRIVATE include)
target_link_libraries(test_parser_exe PRIVATE utl unity::framework)
target_compile_definitions(test_parser_exe PRIVATE UNITY_INCLUDE_DOUBLE)
target_compile_options(test_parser_exe PRIVATE -fpermissive -fsanitize=address)
target_link_options(test_parser_exe PRIVATE -fsanitize=address)
add_test(test_parser test_parser_exe)

add_executable(test_encoding_decoding_exe tests/test_encoding_decoding.c)
target_include_directories(test_encoding_decoding_exe PRIVATE include)
target_link_libraries(test_encoding_decoding_exe PRIVATE utl unity::framework)
target_compile_definitions(test_encoding_decoding_exe PRIVATE UNITY_INCLUDE_DOUBLE)
target_compile_options(test_encoding_decoding_exe PRIVATE -fpermissive -fsanitize=address)
target_link_options(test_encoding_decoding_exe PRIVATE -fsanitize=address)
add_test(test_encoding_decoding test_encoding_decoding_exe)