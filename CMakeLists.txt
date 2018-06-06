project(decode)

cmake_minimum_required(VERSION 2.8.11)

include(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/bmcl/cmake/Bmcl.cmake)

bmcl_add_dep_bmcl(thirdparty/bmcl)
bmcl_add_dep_tclap(thirdparty/tclap)

find_package(Threads)

#zpaq

add_library(zpaq STATIC
    thirdparty/zpaq/libzpaq.cpp
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

else()
    target_compile_definitions(zpaq PRIVATE NOJIT)
endif()

bmcl_disable_warnings(zpaq)

#decode

set(DECODE_CORE_SRC
    src/decode/core/CfgOption.cpp
    src/decode/core/CfgOption.h
    src/decode/core/CmdCallAttr.cpp
    src/decode/core/CmdCallAttr.h
    src/decode/core/Configuration.cpp
    src/decode/core/Configuration.h
    src/decode/core/DataReader.cpp
    src/decode/core/DataReader.h
    src/decode/core/EncodedSizes.h
    src/decode/core/EncodedSizes.cpp
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
    src/decode/core/StringBuilder.cpp
    src/decode/core/StringBuilder.h
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
    src/decode/generator/GcMsgGen.cpp
    src/decode/generator/GcMsgGen.h
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
    src/decode/generator/ReportGen.cpp
    src/decode/generator/ReportGen.h
    src/decode/generator/SrcBuilder.cpp
    src/decode/generator/SrcBuilder.h
    src/decode/generator/StatusEncoderGen.cpp
    src/decode/generator/StatusEncoderGen.h
    src/decode/generator/TypeDefGen.cpp
    src/decode/generator/TypeDefGen.h
    src/decode/generator/TypeDependsCollector.cpp
    src/decode/generator/TypeDependsCollector.h
    src/decode/generator/TypeNameGen.cpp
    src/decode/generator/TypeNameGen.h
    src/decode/generator/TypeReprGen.cpp
    src/decode/generator/TypeReprGen.h
    src/decode/generator/Utils.cpp
    src/decode/generator/Utils.h
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

bmcl_add_library(decode
    src/decode/Config.h
    ${DECODE_CORE_SRC}
    ${DECODE_AST_SRC}
    ${DECODE_GENERATOR_SRC}
    ${DECODE_PARSER_SRC}
)

target_link_libraries(decode
    zpaq
    bmcl
    tclap
    Threads::Threads
)

bmcl_add_executable(decode-gen
    src/decode/Main.cpp
)

target_link_libraries(decode-gen
    decode
    tclap
)

target_compile_definitions(decode PRIVATE -DBUILDING_DECODE)

target_include_directories(decode
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

bmcl_include_system_private_directories(decode
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    thirdparty/zpaq
    thirdparty/pegtl/include
)

#tests
get_directory_property(HAS_PARENT_SCOPE PARENT_DIRECTORY)
if(NOT HAS_PARENT_SCOPE)
    bmcl_add_dep_gtest(thirdparty/gtest)
    add_subdirectory(tests)
endif()

