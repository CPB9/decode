core_src = [
  'core/CfgOption.cpp',
  'core/CfgOption.h',
  'core/CmdCallAttr.cpp',
  'core/CmdCallAttr.h',
  'core/Configuration.cpp',
  'core/Configuration.h',
  'core/DataReader.cpp',
  'core/DataReader.h',
  'core/EncodedSizes.h',
  'core/EncodedSizes.cpp',
  'core/Configuration.h',
  'core/Diagnostics.cpp',
  'core/Diagnostics.h',
  'core/FileInfo.cpp',
  'core/FileInfo.h',
  'core/Foreach.h',
  'core/Hash.h',
  'core/Iterator.h',
  'core/Location.h',
  'core/NamedRc.h',
  'core/PathUtils.cpp',
  'core/PathUtils.h',
  'core/ProgressPrinter.cpp',
  'core/ProgressPrinter.h',
  'core/RangeAttr.cpp',
  'core/RangeAttr.h',
  'core/Rc.h',
  'core/StringBuilder.cpp',
  'core/StringBuilder.h',
  'core/Try.h',
  'core/Utils.h',
  'core/Utils.cpp',
  'core/Zpaq.cpp',
  'core/Zpaq.h',
]

ast_src = [
  'ast/Ast.cpp',
  'ast/Ast.h',
  'ast/AstVisitor.h',
  'ast/Constant.cpp',
  'ast/Constant.h',
  'ast/CmdTrait.cpp',
  'ast/CmdTrait.h',
  'ast/Decl.cpp',
  'ast/Decl.h',
  'ast/Component.cpp',
  'ast/Component.h',
  'ast/DocBlock.cpp',
  'ast/DocBlock.h',
  'ast/DocBlockMixin.cpp',
  'ast/DocBlockMixin.h',
  'ast/Field.cpp',
  'ast/Field.h',
  'ast/Function.cpp',
  'ast/Function.h',
  'ast/ModuleInfo.cpp',
  'ast/ModuleInfo.h',
  'ast/Type.cpp',
  'ast/Type.h',
]

generatos_src = [
  'generator/CmdDecoderGen.cpp',
  'generator/CmdDecoderGen.h',
  'generator/CmdEncoderGen.cpp',
  'generator/CmdEncoderGen.h',
  'generator/DynArrayCollector.cpp',
  'generator/DynArrayCollector.h',
  'generator/FuncPrototypeGen.cpp',
  'generator/FuncPrototypeGen.h',
  'generator/GcInterfaceGen.cpp',
  'generator/GcInterfaceGen.h',
  'generator/GcMsgGen.cpp',
  'generator/GcMsgGen.h',
  'generator/GcTypeGen.cpp',
  'generator/GcTypeGen.h',
  'generator/Generator.cpp',
  'generator/Generator.h',
  'generator/IncludeGen.cpp',
  'generator/IncludeGen.h',
  'generator/InlineSerContext.h',
  'generator/InlineTypeInspector.cpp',
  'generator/InlineTypeInspector.h',
  'generator/NameVisitor.h',
  'generator/OnboardTypeHeaderGen.cpp',
  'generator/OnboardTypeHeaderGen.h',
  'generator/OnboardTypeSourceGen.cpp',
  'generator/OnboardTypeSourceGen.h',
  'generator/SrcBuilder.cpp',
  'generator/SrcBuilder.h',
  'generator/StatusEncoderGen.cpp',
  'generator/StatusEncoderGen.h',
  'generator/TypeDefGen.cpp',
  'generator/TypeDefGen.h',
  'generator/TypeDependsCollector.cpp',
  'generator/TypeDependsCollector.h',
  'generator/TypeNameGen.cpp',
  'generator/TypeNameGen.h',
  'generator/TypeReprGen.cpp',
  'generator/TypeReprGen.h',
  'generator/Utils.cpp',
  'generator/Utils.h',
]

parser_src = [
  'parser/Containers.cpp',
  'parser/Containers.h',
  'parser/Lexer.cpp',
  'parser/Lexer.h',
  'parser/Package.cpp',
  'parser/Package.h',
  'parser/Parser.cpp',
  'parser/Parser.h',
  'parser/Project.cpp',
  'parser/Project.h',
  'parser/Token.h',
]

inc = include_directories('..')

bmcl = subproject('bmcl', default_options: ['build_tests=false'])
zpaq = subproject('zpaq')
pegtl = subproject('pegtl')
toml11 = subproject('toml11')
tclap = subproject('tclap')

thread_dep = dependency('threads')
deps = [
  thread_dep,
  bmcl.get_variable('bmcl_dep'),
  zpaq.get_variable('libzpaq_dep'),
  pegtl.get_variable('pegtl_dep'),
  toml11.get_variable('toml11_dep'),
]

libdecode_lib = static_library('decode',
  sources: ['Config.h'] + core_src + parser_src + ast_src + generatos_src,
  name_prefix: 'lib',
  include_directories: inc,
  dependencies: deps,
  cpp_args: ['-DBUILDING_DECODE']
)

libdecode_dep = declare_dependency(
  link_with: libdecode_lib,
  include_directories: inc,
  dependencies: deps,
)

decode_gen = executable('decode-gen',
  sources: 'Main.cpp',
  dependencies: [libdecode_dep, tclap.get_variable('tclap_dep')],
)

