project(decode)

cmake_minimum_required(VERSION 2.8.11)

if(NOT MSVC)
    add_definitions()
endif()

macro(enable_asan)
    add_definitions(
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize-address-use-after-scope
        -fno-common
        -fno-omit-frame-pointer
        -ggdb
        #-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizer-blacklist.txt
    )
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endmacro()

macro(enable_msan)
    add_definitions(
        -fsanitize=memory
        -fno-common
        -fno-omit-frame-pointer
        -ggdb
        #-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizer-blacklist.txt
    )
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
endmacro()

if(ASAN)
    enable_asan()
endif()

if(MSAN)
    enable_msan()
endif()

add_subdirectory(thirdparty/bmcl EXCLUDE_FROM_ALL)

include_directories(
    thirdparty/zpaq
    thirdparty/tclap/include
    thirdparty
    thirdparty/pegtl
)

add_library(zpaq STATIC
    thirdparty/zpaq/libzpaq.cpp
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

else()
    target_compile_definitions(zpaq PRIVATE NOJIT)
endif()

find_package(Threads)

set(DECODE_CORE_SRC
    src/decode/core/CfgOption.cpp
    src/decode/core/CfgOption.h
    src/decode/core/Configuration.cpp
    src/decode/core/Configuration.h
    src/decode/core/DataReader.cpp
    src/decode/core/DataReader.h
    src/decode/core/Configuration.h
    src/decode/core/Diagnostics.cpp
    src/decode/core/Diagnostics.h
    src/decode/core/FileInfo.cpp
    src/decode/core/FileInfo.h
    src/decode/core/Foreach.h
    src/decode/core/Hash.h
    src/decode/core/Iterator.h
    src/decode/core/Location.h
    src/decode/core/NamedRc.h
    src/decode/core/PathUtils.cpp
    src/decode/core/PathUtils.h
    src/decode/core/ProgressPrinter.cpp
    src/decode/core/ProgressPrinter.h
    src/decode/core/RangeAttr.cpp
    src/decode/core/RangeAttr.h
    src/decode/core/Rc.h
    src/decode/core/Try.h
    src/decode/core/Utils.h
    src/decode/core/Utils.cpp
    src/decode/core/Zpaq.cpp
    src/decode/core/Zpaq.h
)
source_group("core" FILES ${DECODE_CORE_SRC})

set(DECODE_AST_SRC
    src/decode/ast/Ast.cpp
    src/decode/ast/Ast.h
    src/decode/ast/AstVisitor.h
    src/decode/ast/Constant.cpp
    src/decode/ast/Constant.h
    src/decode/ast/CmdTrait.cpp
    src/decode/ast/CmdTrait.h
    src/decode/ast/Decl.cpp
    src/decode/ast/Decl.h
    src/decode/ast/Component.cpp
    src/decode/ast/Component.h
    src/decode/ast/DocBlock.cpp
    src/decode/ast/DocBlock.h
    src/decode/ast/DocBlockMixin.cpp
    src/decode/ast/DocBlockMixin.h
    src/decode/ast/Field.cpp
    src/decode/ast/Field.h
    src/decode/ast/Function.cpp
    src/decode/ast/Function.h
    src/decode/ast/ModuleInfo.cpp
    src/decode/ast/ModuleInfo.h
    src/decode/ast/Type.cpp
    src/decode/ast/Type.h
)
source_group("ast" FILES ${DECODE_AST_SRC})

set(DECODE_GENERATOR_SRC
    src/decode/generator/CmdDecoderGen.cpp
    src/decode/generator/CmdDecoderGen.h
    src/decode/generator/CmdEncoderGen.cpp
    src/decode/generator/CmdEncoderGen.h
    src/decode/generator/DynArrayCollector.cpp
    src/decode/generator/DynArrayCollector.h
    src/decode/generator/FuncPrototypeGen.cpp
    src/decode/generator/FuncPrototypeGen.h
    src/decode/generator/GcInterfaceGen.cpp
    src/decode/generator/GcInterfaceGen.h
    src/decode/generator/GcStatusMsgGen.cpp
    src/decode/generator/GcStatusMsgGen.h
    src/decode/generator/GcTypeGen.cpp
    src/decode/generator/GcTypeGen.h
    src/decode/generator/Generator.cpp
    src/decode/generator/Generator.h
    src/decode/generator/IncludeGen.cpp
    src/decode/generator/IncludeGen.h
    src/decode/generator/InlineSerContext.h
    src/decode/generator/InlineTypeInspector.cpp
    src/decode/generator/InlineTypeInspector.h
    src/decode/generator/NameVisitor.h
    src/decode/generator/OnboardTypeHeaderGen.cpp
    src/decode/generator/OnboardTypeHeaderGen.h
    src/decode/generator/OnboardTypeSourceGen.cpp
    src/decode/generator/OnboardTypeSourceGen.h
    src/decode/generator/SrcBuilder.cpp
    src/decode/generator/SrcBuilder.h
    src/decode/generator/StatusEncoderGen.cpp
    src/decode/generator/StatusEncoderGen.h
    src/decode/generator/StringBuilder.cpp
    src/decode/generator/StringBuilder.h
    src/decode/generator/TypeDefGen.cpp
    src/decode/generator/TypeDefGen.h
    src/decode/generator/TypeDependsCollector.cpp
    src/decode/generator/TypeDependsCollector.h
    src/decode/generator/TypeNameGen.cpp
    src/decode/generator/TypeNameGen.h
    src/decode/generator/TypeReprGen.cpp
    src/decode/generator/TypeReprGen.h
)
source_group("generator" FILES ${DECODE_GENERATOR_SRC})

set(DECODE_PARSER_SRC
    src/decode/parser/Containers.cpp
    src/decode/parser/Containers.h
    src/decode/parser/Lexer.cpp
    src/decode/parser/Lexer.h
    src/decode/parser/Package.cpp
    src/decode/parser/Package.h
    src/decode/parser/Parser.cpp
    src/decode/parser/Parser.h
    src/decode/parser/Project.cpp
    src/decode/parser/Project.h
    src/decode/parser/Token.h
)
source_group("parser" FILES ${DECODE_PARSER_SRC})

add_library(decode
    src/decode/Config.h
    ${DECODE_CORE_SRC}
    ${DECODE_AST_SRC}
    ${DECODE_GENERATOR_SRC}
    ${DECODE_PARSER_SRC}
)

target_link_libraries(decode zpaq bmcl Threads::Threads)

add_executable(decode_gen
    src/decode/Main.cpp
)

target_link_libraries(decode_gen decode)

set_target_properties(decode
    PROPERTIES
    PREFIX "lib"
)

if(Qt5Widgets_FOUND)

endif()

target_compile_definitions(decode PRIVATE -DBUILDING_DECODE)

if(NOT MSVC)
    target_compile_options(decode PUBLIC -std=c++11)
    target_compile_options(decode PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(zpaq PRIVATE -w)
endif()

target_include_directories(decode
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

get_directory_property(HAS_PARENT_SCOPE PARENT_DIRECTORY)
if(NOT HAS_PARENT_SCOPE)
    add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()

set_target_properties(decode decode_gen
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS decode decode_gen
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
