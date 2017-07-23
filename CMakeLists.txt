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

find_package(Qt5Widgets)

# caf

set(CAF_NO_EXAMPLES 1)
set(CAF_NO_BENCHMARKS 1)
set(CAF_NO_PYTHON 1)
set(CAF_NO_OPENCL 1)
set(CAF_NO_TOOLS 1)
set(CAF_NO_UNIT_TESTS 1)
set(CAF_LOG_LEVEL 0)
if (UNIX)
    set(CAF_CORE_LIBRARIES libcaf_core_shared)
    set(CAF_IO_LIBRARIES libcaf_io_shared)
else()
    set(CAF_CORE_LIBRARIES libcaf_core_static)
    set(CAF_IO_LIBRARIES libcaf_io_static)
endif()
if (NOT MSVC)
    set(_TMP_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "")
endif()
add_subdirectory(thirdparty/caf)
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS ${_TMP_FLAGS})
endif()
target_include_directories(${CAF_CORE_LIBRARIES} SYSTEM INTERFACE thirdparty/caf/libcaf_core)
target_include_directories(${CAF_IO_LIBRARIES} SYSTEM INTERFACE thirdparty/caf/libcaf_io)

#caf

set(DECODE_UI_SRC)

if(Qt5Widgets_FOUND)
    set(DECODE_UI_HEADERS
        src/decode/ui/FirmwareWidget.h
        src/decode/ui/FirmwareStatusWidget.h
        src/decode/ui/QModelBase.h
        src/decode/ui/QCmdModel.h
        src/decode/ui/QNodeModel.h
        src/decode/ui/QNodeViewModel.h
        src/decode/ui/QModelEventHandler.h
    )
    qt5_wrap_cpp(DECODE_UI_MOC
        ${DECODE_UI_HEADERS}
    )
    set(DECODE_UI_SRC
        src/decode/ui/FirmwareWidget.cpp
        src/decode/ui/FirmwareStatusWidget.cpp
        src/decode/ui/QCmdModel.cpp
        src/decode/ui/QNodeModel.cpp
        src/decode/ui/QNodeViewModel.cpp
        src/decode/ui/QModelEventHandler.cpp
        ${DECODE_UI_MOC}
    )
    source_group("ui" FILES ${DECODE_UI_SRC})
    source_group("ui_moc" FILES ${DECODE_UI_MOC})
endif()

set(DECODE_CORE_SRC
    src/decode/core/CfgOption.cpp
    src/decode/core/CfgOption.h
    src/decode/core/Configuration.cpp
    src/decode/core/Configuration.h
    src/decode/core/Diagnostics.cpp
    src/decode/core/Diagnostics.h
    src/decode/core/FileInfo.h
    src/decode/core/Hash.h
    src/decode/core/Location.h
    src/decode/core/Rc.h
    src/decode/core/PathUtils.cpp
    src/decode/core/PathUtils.h
    src/decode/core/Zpaq.cpp
    src/decode/core/Zpaq.h
)
source_group("core" FILES ${DECODE_CORE_SRC})

set(DECODE_GENERATOR_SRC
    src/decode/generator/CmdDecoderGen.cpp
    src/decode/generator/CmdDecoderGen.h
    src/decode/generator/CmdEncoderGen.cpp
    src/decode/generator/CmdEncoderGen.h
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
    src/decode/generator/TypeReprGen.h
)
source_group("generator" FILES ${DECODE_GENERATOR_SRC})

set(DECODE_GROUNDCONTROL_SRC
    src/decode/groundcontrol/Crc.cpp
    src/decode/groundcontrol/Crc.h
    src/decode/groundcontrol/Exchange.cpp
    src/decode/groundcontrol/Exchange.h
    src/decode/groundcontrol/FwtState.cpp
    src/decode/groundcontrol/FwtState.h
    src/decode/groundcontrol/GroundControl.cpp
    src/decode/groundcontrol/GroundControl.h
    src/decode/groundcontrol/MemIntervalSet.cpp
    src/decode/groundcontrol/MemIntervalSet.h
    src/decode/groundcontrol/TmState.cpp
    src/decode/groundcontrol/TmState.h
)
source_group("groundcontrol" FILES ${DECODE_GROUNDCONTROL_SRC})

set(DECODE_MODEL_SRC
    src/decode/model/CmdNode.cpp
    src/decode/model/CmdNode.h
    src/decode/model/Decoder.cpp
    src/decode/model/Decoder.h
    src/decode/model/FieldsNode.cpp
    src/decode/model/FieldsNode.h
    src/decode/model/ModelEventHandler.cpp
    src/decode/model/ModelEventHandler.h
    src/decode/model/Model.cpp
    src/decode/model/Model.h
    src/decode/model/TmModel.cpp
    src/decode/model/TmModel.h
    src/decode/model/Node.cpp
    src/decode/model/Node.h
    src/decode/model/Value.cpp
    src/decode/model/Value.h
    src/decode/model/ValueInfoCache.cpp
    src/decode/model/ValueInfoCache.h
    src/decode/model/ValueKind.h
    src/decode/model/ValueNode.cpp
    src/decode/model/ValueNode.h
)
source_group("model" FILES ${DECODE_MODEL_SRC})

set(DECODE_PARSER_SRC
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
    src/decode/parser/Project.cpp
    src/decode/parser/Project.h
    src/decode/parser/Span.h
    src/decode/parser/Token.h
    src/decode/parser/Token.h
)
source_group("parser" FILES ${DECODE_PARSER_SRC})

add_library(decode
    src/decode/Config.h
    ${DECODE_CORE_SRC}
    ${DECODE_GENERATOR_SRC}
    ${DECODE_GROUNDCONTROL_SRC}
    ${DECODE_MODEL_SRC}
    ${DECODE_PARSER_SRC}
    ${DECODE_UI_SRC}
)

target_link_libraries(decode zpaq ${CAF_CORE_LIBRARIES})

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
