core_src = [
  'core/CfgOption.cpp',
  'core/CmdCallAttr.cpp',
  'core/Configuration.cpp',
  'core/DataReader.cpp',
  'core/EncodedSizes.cpp',
  'core/Diagnostics.cpp',
  'core/FileInfo.cpp',
  'core/PathUtils.cpp',
  'core/ProgressPrinter.cpp',
  'core/RangeAttr.cpp',
  'core/StringBuilder.cpp',
  'core/Utils.cpp',
  'core/Zpaq.cpp',
]

ast_src = [
  'ast/Ast.cpp',
  'ast/Constant.cpp',
  'ast/CmdTrait.cpp',
  'ast/Decl.cpp',
  'ast/Component.cpp',
  'ast/DocBlock.cpp',
  'ast/DocBlockMixin.cpp',
  'ast/Field.cpp',
  'ast/Function.cpp',
  'ast/ModuleInfo.cpp',
  'ast/Type.cpp',
]

generatos_src = [
  'generator/CmdDecoderGen.cpp',
  'generator/CmdEncoderGen.cpp',
  'generator/DynArrayCollector.cpp',
  'generator/FuncPrototypeGen.cpp',
  'generator/GcInterfaceGen.cpp',
  'generator/GcMsgGen.cpp',
  'generator/GcTypeGen.cpp',
  'generator/Generator.cpp',
  'generator/IncludeGen.cpp',
  'generator/InlineTypeInspector.cpp',
  'generator/OnboardTypeHeaderGen.cpp',
  'generator/OnboardTypeSourceGen.cpp',
  'generator/ReportGen.cpp',
  'generator/SrcBuilder.cpp',
  'generator/StatusEncoderGen.cpp',
  'generator/TypeDefGen.cpp',
  'generator/TypeDependsCollector.cpp',
  'generator/TypeNameGen.cpp',
  'generator/TypeReprGen.cpp',
  'generator/Utils.cpp',
]

parser_src = [
  'parser/Containers.cpp',
  'parser/Lexer.cpp',
  'parser/Package.cpp',
  'parser/Parser.cpp',
  'parser/Project.cpp',
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
  sources: core_src + parser_src + ast_src + generatos_src,
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

