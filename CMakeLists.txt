project(decode)

cmake_minimum_required(VERSION 2.8.11)

add_subdirectory(thirdparty/bmcl EXCLUDE_FROM_ALL)

add_library(decode
    src/decode/Config.h
    src/decode/Rc.h
    src/decode/parser/Decl.h
    src/decode/parser/Location.h
    src/decode/parser/Parser.cpp
    src/decode/parser/Parser.h
    src/decode/parser/Lexer.cpp
    src/decode/parser/Lexer.h
    src/decode/parser/ParseTree.cpp
    src/decode/parser/ParseTree.h
    src/decode/parser/Span.h
    src/decode/parser/Token.h
)

add_executable(dummy
    dummy.cpp
)

target_link_libraries(dummy decode)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
add_subdirectory(tests)
