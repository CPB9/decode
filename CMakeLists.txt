project(decode)

cmake_minimum_required(VERSION 2.8.11)

if(NOT MSVC)
    add_definitions()
endif()

macro(enable_sanitizer)
    add_definitions(
        -fsanitize=address
        -fsanitize=integer
        -fsanitize=undefined
        -ggdb
        #-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizer-blacklist.txt
    )
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=integer -fsanitize=undefined")
endmacro()

if(ASAN)
    enable_sanitizer()
endif()

add_subdirectory(thirdparty/bmcl EXCLUDE_FROM_ALL)

include_directories(
    thirdparty/zpaq
    thirdparty/tclap/include
)

add_library(zpaq
    thirdparty/zpaq/libzpaq.cpp
)

add_library(decode
    src/decode/Config.h
    src/decode/core/Diagnostics.cpp
    src/decode/core/Diagnostics.h
    src/decode/core/FileInfo.h
    src/decode/core/Hash.h
    src/decode/core/Location.h
    src/decode/core/Rc.h
    src/decode/core/StringSwitch.h
    src/decode/parser/Ast.cpp
    src/decode/parser/Ast.h
    src/decode/parser/Decl.h
    src/decode/parser/Parser.cpp
    src/decode/parser/Parser.h
    src/decode/parser/Lexer.cpp
    src/decode/parser/Lexer.h
    src/decode/parser/Span.h
    src/decode/parser/Token.h
    src/decode/parser/Package.cpp
    src/decode/parser/Package.h
    src/decode/generator/Generator.cpp
    src/decode/generator/Generator.h
    src/decode/generator/SliceNameGenerator.cpp
    src/decode/generator/SliceNameGenerator.h
    src/decode/generator/TypeReprGenerator.cpp
    src/decode/generator/TypeReprGenerator.h
)

target_link_libraries(decode zpaq)

add_executable(decode_gen
    src/decode/Main.cpp
)

target_link_libraries(decode_gen decode)

set_target_properties(decode
    PROPERTIES
    PREFIX "lib"
)

target_link_libraries(decode bmcl)

target_compile_definitions(decode PRIVATE -DBUILDING_DECODE)

if(NOT MSVC)
    target_compile_options(decode PUBLIC -std=c++11)
    target_compile_options(decode PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(zpaq PRIVATE -w)
endif()

target_include_directories(decode
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pegtl
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
add_subdirectory(tests)
