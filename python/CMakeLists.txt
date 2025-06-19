set(CMAKE_CXX_STANDARD 20)

set(Python_FIND_STRATEGY LOCATION)
set(Python3_ROOT_DIR /usr)
set(Python3_FIND_VIRTUALENV ONLY)

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)

add_library(pyutl SHARED
        src/def_pool.c
        src/tltype.c
        src/tlobject.c
        src/tlvector.c
        src/pyutl.c)
target_include_directories(pyutl PUBLIC include ${Python3_INCLUDE_DIRS})
target_link_libraries(pyutl PUBLIC utl ${Python3_LIBRARIES})
target_compile_definitions(pyutl PUBLIC PY_SSIZE_T_CLEAN)
set_target_properties(pyutl PROPERTIES PREFIX "")
set_target_properties(pyutl PROPERTIES OUTPUT_NAME "_pyutl.${Python3_SOABI}")
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(pyutl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../pyutl")
endif ()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_options(pyutl PUBLIC -Og -g -ggdb)
    #target_compile_options(pyutl PUBLIC -fPIC -fpermissive -fsanitize=address)
    #target_link_options(pyutl PRIVATE -fsanitize=address)
else()
    target_compile_options(pyutl PUBLIC -fPIC -Os -s)
endif()

add_custom_command(
        TARGET pyutl
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PROJECT_SOURCE_DIR} CMAKE_POST_BUILD_TEST=1 ${Python3_EXECUTABLE} -m pytest test_everything.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)