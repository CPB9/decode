project(decode)

cmake_minimum_required(VERSION 2.8.11)

add_subdirectory(thirdparty/bmcl EXCLUDE_FROM_ALL)

add_library(decode
    src/decode/Grammar.h
    src/decode/Parser.cpp
    src/decode/Parser.h
)

set_target_properties(decode
    PROPERTIES
    PREFIX "lib"
)

target_link_libraries(decode bmcl)

target_compile_definitions(decode PRIVATE -DBUILDING_DECODE)

if(NOT MSVC)
    target_compile_options(decode PUBLIC -std=c++11)
endif()

target_include_directories(decode
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pegtl
)

add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
add_subdirectory(tests)
