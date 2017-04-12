project(decode)

cmake_minimum_required(VERSION 2.8.11)

if(NOT MSVC)
    add_definitions()
endif()

macro(enable_sanitizer)
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

if(ASAN)
    enable_sanitizer()
endif()

add_subdirectory(thirdparty/bmcl EXCLUDE_FROM_ALL)

include_directories(
    thirdparty/zpaq
    thirdparty/tclap/include
)

add_library(zpaq STATIC
    thirdparty/zpaq/libzpaq.cpp
)

find_package(Qt5Widgets)

set(DECODE_UI_SRC)

if(Qt5Widgets_FOUND)
    set(DECODE_UI_HEADERS
        src/decode/ui/QModel.h
        src/decode/ui/QModelEventHandler.h
    )
    qt5_wrap_cpp(DECODE_UI_MOC
        ${DECODE_UI_HEADERS}
    )
    set(DECODE_UI_SRC
        src/decode/ui/QModel.cpp
        src/decode/ui/QModelEventHandler.cpp
        ${DECODE_UI_MOC}
    )
endif()

add_library(decode
    src/decode/Config.h
    src/decode/core/Diagnostics.cpp
    src/decode/core/Diagnostics.h
    src/decode/core/FileInfo.h
    src/decode/core/Hash.h
    src/decode/core/Location.h
    src/decode/core/Rc.h
    src/decode/core/Configuration.cpp
    src/decode/core/Configuration.h
    src/decode/core/CfgOption.cpp
    src/decode/core/CfgOption.h
    src/decode/generator/Generator.cpp
    src/decode/generator/Generator.h
    src/decode/generator/HeaderGen.cpp
    src/decode/generator/HeaderGen.h
    src/decode/generator/IncludeCollector.cpp
    src/decode/generator/IncludeCollector.h
    src/decode/generator/InlineSerContext.h
    src/decode/generator/InlineTypeDeserializerGen.cpp
    src/decode/generator/InlineTypeDeserializerGen.h
    src/decode/generator/InlineTypeSerializerGen.cpp
    src/decode/generator/InlineTypeSerializerGen.h
    src/decode/generator/NameVisitor.h
    src/decode/generator/SourceGen.cpp
    src/decode/generator/SourceGen.h
    src/decode/generator/SrcBuilder.cpp
    src/decode/generator/SrcBuilder.h
    src/decode/generator/StatusEncoderGen.cpp
    src/decode/generator/StatusEncoderGen.h
    src/decode/generator/StringBuilder.h
    src/decode/generator/TypeDefGen.cpp
    src/decode/generator/TypeDefGen.h
    src/decode/generator/TypeNameGen.cpp
    src/decode/generator/TypeNameGen.h
    src/decode/generator/TypeReprGen.cpp
    src/decode/generator/TypeReprGen.h
    src/decode/generator/CmdDecoderGen.cpp
    src/decode/generator/CmdDecoderGen.h
    src/decode/generator/TypeReprGen.h
    src/decode/model/Decoder.cpp
    src/decode/model/Decoder.h
    src/decode/model/FieldsNode.cpp
    src/decode/model/FieldsNode.h
    src/decode/model/Model.cpp
    src/decode/model/Model.h
    src/decode/model/Node.cpp
    src/decode/model/Node.h
    src/decode/model/Value.cpp
    src/decode/model/Value.h
    src/decode/model/ValueKind.h
    src/decode/model/ValueInfoCache.cpp
    src/decode/model/ValueInfoCache.h
    src/decode/model/ValueNode.cpp
    src/decode/model/ValueNode.h
    src/decode/parser/Ast.cpp
    src/decode/parser/Ast.h
    src/decode/parser/Containers.cpp
    src/decode/parser/Containers.h
    src/decode/parser/Decl.h
    src/decode/parser/Lexer.cpp
    src/decode/parser/Lexer.h
    src/decode/parser/Package.cpp
    src/decode/parser/Package.h
    src/decode/parser/Parser.cpp
    src/decode/parser/Parser.h
    src/decode/parser/Span.h
    src/decode/parser/Token.h
    ${DECODE_UI_SRC}
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

if(Qt5Widgets_FOUND)
    target_link_libraries(decode Qt5::Widgets)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pegtl
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)


get_directory_property(HAS_PARENT_SCOPE PARENT_DIRECTORY)
if(NOT HAS_PARENT_SCOPE)
    add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()

if(WIN32)
    get_target_property(_TARGET_LOCATION bmcl LOCATION)
    add_custom_command(
        TARGET decode
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bmcl>
        ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

