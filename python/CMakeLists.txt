set(CMAKE_CXX_STANDARD 20)

set(Python_FIND_STRATEGY LOCATION)
set(Python3_ROOT_DIR /usr)

find_package(Python3 3.10 COMPONENTS Interpreter Development.Module REQUIRED)

add_library(pyutl SHARED
        src/def_pool.c
        src/ptr_map.c
        src/tlobject.c
        src/tlvector.c
        src/pyutl.c)
target_include_directories(pyutl PUBLIC include ${Python3_INCLUDE_DIRS})
target_link_libraries(pyutl PUBLIC utl ${Python3_LIBRARIES})
target_compile_definitions(pyutl PUBLIC PY_SSIZE_T_CLEAN)
set_target_properties(pyutl PROPERTIES PREFIX "")
set_target_properties(pyutl PROPERTIES OUTPUT_NAME "_pyutl")
set_target_properties(pyutl PROPERTIES SUFFIX ".${Python3_SOABI}.so")
set_target_properties(pyutl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pyutl")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pyutl PUBLIC -fPIC)
    #target_compile_options(pyutl PUBLIC -fPIC -fpermissive -fsanitize=address)
    #target_link_options(pyutl PRIVATE -fsanitize=address)
else()
    target_compile_options(pyutl PUBLIC -fPIC -Os -s)
endif()