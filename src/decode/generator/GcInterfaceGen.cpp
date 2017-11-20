#include "decode/generator/GcInterfaceGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/parser/Package.h"
#include "decode/ast/Ast.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/generator/SrcBuilder.h"

namespace decode {

GcInterfaceGen::GcInterfaceGen(SrcBuilder* dest)
    : _output(dest)
{
}

GcInterfaceGen::~GcInterfaceGen()
{
}

void GcInterfaceGen::generateHeader(const Package* package)
{
    TypeDependsCollector coll;
    TypeDependsCollector::Depends depends;
    for (const Ast* ast : package->modules()) {
        coll.collect(ast, &depends);
    }
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(&depends);

    _output->append(
                    "#include <decode/ast/Ast.h>\n"
                    "#include <decode/ast/Component.h>\n"
                    "#include <decode/ast/Utils.h>\n"
                    "#include <decode/ast/Type.h>\n"
                    "#include <decode/ast/AllBuiltinTypes.h>\n"
                    "#include <decode/ast/Field.h>\n"
                    "#include <decode/parser/Project.h>\n"
                    "class Validator {\npublic:\n"
                    "    Validator(const decode::Project* project, const decode::Device* device)\n"
                    "        : _project(project)\n"
                    "        , _device(device)\n"
                    "    {\n");

    for (const Ast* ast : package->modules()) {
        _output->append("        _");
        _output->append(ast->moduleName());
        _output->append("Ast = decode::findModule(_device.get(), \"");
        _output->append(ast->moduleName());
        _output->append("\");\n        _");
        _output->append(ast->moduleName());
        _output->append("Component = decode::getComponent(_");
        _output->append(ast->moduleName());
        _output->append("Ast.get());\n");
    }
    _output->appendEol();

    _output->append(
                    "        if (!_coreAst) {\n            return;\n        }\n"
                    "        _builtinUsize = _coreAst->builtinTypes()->usizeType();\n"
                    "        _builtinIsize = _coreAst->builtinTypes()->isizeType();\n"
                    "        _builtinU8 = _coreAst->builtinTypes()->u8Type();\n"
                    "        _builtinU16 = _coreAst->builtinTypes()->u16Type();\n"
                    "        _builtinU32 = _coreAst->builtinTypes()->u32Type();\n"
                    "        _builtinU64 = _coreAst->builtinTypes()->u64Type();\n"
                    "        _builtinI8 = _coreAst->builtinTypes()->i8Type();\n"
                    "        _builtinI16 = _coreAst->builtinTypes()->i16Type();\n"
                    "        _builtinI32 = _coreAst->builtinTypes()->i32Type();\n"
                    "        _builtinI64 = _coreAst->builtinTypes()->i64Type();\n"
                    "        _builtinF32 = _coreAst->builtinTypes()->f32Type();\n"
                    "        _builtinF64 = _coreAst->builtinTypes()->f64Type();\n"
                    "        _builtinVaruint = _coreAst->builtinTypes()->varuintType();\n"
                    "        _builtinVarint = _coreAst->builtinTypes()->varintType();\n"
                    "        _builtinBool = _coreAst->builtinTypes()->boolType();\n"
                    "        _builtinVoid = _coreAst->builtinTypes()->voidType();\n"
                    "        _builtinChar = _coreAst->builtinTypes()->charType();\n\n");

    for (const Ast* ast : package->modules()) {
        for (const Type* type : ast->typesRange()) {
            appendTypeValidator(type);
        }
    }

    _output->append("    }\n");
    _output->append("private:\n"
                    "    decode::Rc<const decode::Project> _project;\n"
                    "    decode::Rc<const decode::Device> _device;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinUsize;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinIsize;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinU8;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinU16;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinU32;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinU64;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinI8;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinI16;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinI32;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinI64;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinF32;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinF64;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinVaruint;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinVarint;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinBool;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinVoid;\n"
                    "    decode::Rc<const decode::BuiltinType> _builtinChar;\n"
                   );

    for (const Ast* ast : package->modules()) {
        _output->append("    decode::Rc<const decode::Ast> _");
        _output->append(ast->moduleName());
        _output->append("Ast;\n");
        _output->append("    decode::Rc<const decode::Component> _");
        _output->append(ast->moduleName());
        _output->append("Component;\n");
    }

    for (auto pair : _validatedTypes) {
        if (pair.first.size() == 0) {
            continue;
        }
        _output->append("    decode::Rc<");
        switch (pair.second->resolveFinalType()->typeKind()) {
        case TypeKind::Reference:
        case TypeKind::Array:
        case TypeKind::Function:
        case TypeKind::DynArray:
            break;
        case TypeKind::Builtin:
        case TypeKind::GenericParameter:
        case TypeKind::GenericInstantiation:
        case TypeKind::Enum:
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Imported:
        case TypeKind::Alias:
        case TypeKind::Generic:
            _output->append("const ");
            break;
        }
        _output->append("decode::");
        _output->append(pair.second->resolveFinalType()->renderTypeKind());
        _output->append("Type> ");
        _output->append(pair.first);
        _output->append(";\n");
    }

    _output->append("};\n\n");
    _validatedTypes.clear();
}

void GcInterfaceGen::appendTypeValidator(const Type* type)
{
    if (type->resolveFinalType()->isGeneric()) {
        return;
    }
    if (type->resolveFinalType()->isBuiltin()) {
        return;
    }
    SrcBuilder nameBuilder;
    appendTestedType(type, &nameBuilder);
    auto pair = _validatedTypes.emplace(nameBuilder.result(), type);
    if (!pair.second) {
        return;
    }
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        break;
    case TypeKind::Reference: {
        appendTypeValidator(type->asReference()->pointee());
        _output->append("        ");
        appendTestedType(type);
        _output->append(" = new decode::ReferenceType(");
        switch (type->asReference()->referenceKind()) {
        case ReferenceKind::Pointer:
            _output->append("decode::ReferenceKind::Pointer, ");
            break;
        case ReferenceKind::Reference:
            _output->append("decode::ReferenceKind::Reference, ");
            break;
        }
        if (type->asReference()->isMutable()) {
            _output->append("true, (decode::Type*)");
        } else {
            _output->append("false, (decode::Type*)");
        }
        appendTestedType(type->asReference()->pointee());
        _output->append(".get());\n\n");
        break;
    }
    case TypeKind::Array:
        appendTypeValidator(type->asArray()->elementType());
        _output->append("        ");
        appendTestedType(type);
        _output->append(" = new decode::ArrayType(");
        _output->appendNumericValue(type->asArray()->elementCount());
        _output->append(", (decode::Type*)");
        appendTestedType(type->asArray()->elementType());
        _output->append(".get());\n\n");
        break;
    case TypeKind::Function: {
        const FunctionType* f = type->asFunction();
        if (f->hasReturnValue()) {
            appendTypeValidator(f->returnValue().unwrap());
        }
        for (const Field* field : f->argumentsRange()) {
            appendTypeValidator(field->type());
        }
        _output->append("        ");
        appendTestedType(type);
        _output->append(" = new decode::FunctionType();\n");
        if (f->hasReturnValue()) {
            _output->append("        ");
            appendTestedType(type);
            _output->append("->setReturnValue((decode::Type*)");
            appendTestedType(f->returnValue().unwrap());
            _output->append(".get());\n");
        }
        if (f->selfArgument().isSome()) {
            _output->append("        ");
            appendTestedType(type);
            _output->append("->setSelfArgument(decode::SelfArgument::");
            switch (f->selfArgument().unwrap()) {
                case SelfArgument::Reference:
                    _output->append("Reference");
                    break;
                case SelfArgument::MutReference:
                    _output->append("MutReference");
                    break;
                case SelfArgument::Value:
                    _output->append("Value");
                    break;
            }
            _output->append(");\n");
        }
        for (const Field* field : f->argumentsRange()) {
            _output->append("        ");
            appendTestedType(type);
            _output->append("->addArgument(new decode::Field(\"\", (decode::Type*)");
            appendTestedType(field->type());
            _output->append(".get()));\n");
        }
        break;
    }
    case TypeKind::GenericParameter:
        break;
    case TypeKind::DynArray:
        appendTypeValidator(type->asDynArray()->elementType());
        _output->append("        ");
        appendTestedType(type);
        _output->append(" = new decode::DynArrayType(");
        _output->appendNumericValue(type->asDynArray()->maxSize());
        _output->append(", (decode::Type*)");
        appendTestedType(type->asDynArray()->elementType());
        _output->append(".get());\n\n");
        break;
    case TypeKind::Enum: {
        appendNamedTypeInit(type->asEnum(), "Enum");
        _output->appendEol();
        break;
    }
    case TypeKind::Struct: {
        for (const Field* field : type->asStruct()->fieldsRange()) {
            appendTypeValidator(field->type());
        }
        appendNamedTypeInit(type->asStruct(), "Struct");
        _output->append("        decode::expectFieldNum(&_");
        _output->append(type->asStruct()->moduleName());
        _output->appendWithFirstUpper(type->asStruct()->name());
        _output->append(", ");
        _output->appendNumericValue(type->asStruct()->fieldsRange().size());
        _output->append(");\n");

        std::size_t i = 0;
        for (const Field* field : type->asStruct()->fieldsRange()) {
            _output->append("        decode::expectField(&_");
            _output->append(type->asStruct()->moduleName());
            _output->appendWithFirstUpper(type->asStruct()->name());
            _output->append(", ");
            _output->appendNumericValue(i);
            _output->append(", \"");
            _output->append(field->name());
            _output->append("\", ");
            appendTestedType(field->type());
            _output->append(".get()");
            _output->append(");\n");
            i++;
        }
        _output->appendEol();
        break;
    }
    case TypeKind::Variant: {
        appendNamedTypeInit(type->asVariant(), "Variant");
        _output->appendEol();
        break;
    }
    case TypeKind::Imported:
        appendTypeValidator(type->asImported()->link());
        break;
    case TypeKind::Alias:
        appendTypeValidator(type->asAlias()->alias());
        break;
    case TypeKind::Generic:
        break;
    case TypeKind::GenericInstantiation:
        break;
    }
}

void GcInterfaceGen::appendTestedType(const Type* type)
{
    appendTestedType(type, _output);
}

void GcInterfaceGen::appendTestedType(const Type* type, SrcBuilder* dest)
{
    auto appendNamed = [dest](const NamedType* type) {
        dest->append('_');
        dest->append(type->moduleName());
        dest->appendWithFirstUpper(type->name());
    };
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        dest->append("_builtin");
        dest->appendWithFirstUpper(type->asBuiltin()->renderedTypeName(type->asBuiltin()->builtinTypeKind()));
        break;
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::GenericParameter:
    case TypeKind::DynArray:
    case TypeKind::GenericInstantiation: {
        TypeNameGen gen(dest);
        dest->append("_");
        gen.genTypeName(type);
        break;
    }
    case TypeKind::Enum:
        appendNamed(type->asEnum());
        break;
    case TypeKind::Struct:
        appendNamed(type->asStruct());
        break;
    case TypeKind::Variant:
        appendNamed(type->asVariant());
        break;
    case TypeKind::Imported:
        appendTestedType(type->asImported()->link(), dest);
        break;
    case TypeKind::Alias:
        appendTestedType(type->asAlias()->alias(), dest);
        break;
    case TypeKind::Generic:
        break;
    }
}

void GcInterfaceGen::appendNamedTypeInit(const NamedType* type, bmcl::StringView name)
{
    _output->append("        _");
    _output->append(type->moduleName());
    _output->appendWithFirstUpper(type->name());
    _output->append(" = decode::findType<decode::");
    _output->append(name);
    _output->append("Type>(_");
    _output->append(type->moduleName());
    _output->append("Ast.get(), \"");
    _output->appendWithFirstUpper(type->name());
    _output->append("\");\n");
}
}
