#include "decode/generator/GcInterfaceGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/InlineTypeInspector.h"
#include "decode/generator/GcMsgGen.h"
#include "decode/parser/Package.h"
#include "decode/ast/Ast.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/core/Foreach.h"
#include "decode/generator/SrcBuilder.h"

//TODO: refact

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
    _output->appendPragmaOnce();
    _output->appendEol();
    TypeDependsCollector coll;
    TypeDependsCollector::Depends depends;
    for (const Ast* ast : package->modules()) {
        coll.collect(ast, &depends);
    }
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(&depends);
    _output->appendEol();

    for (const Component* comp : package->components()) {
        for (const StatusMsg* msg : comp->statusesRange()) {
            _output->append("#include \"photongen/groundcontrol/_statuses_/");
            _output->appendWithFirstUpper(comp->name());
            _output->append("_");
            _output->appendWithFirstUpper(msg->name());
            _output->append(".hpp\"\n");
        }
    }
    _output->appendEol();

    for (const Component* comp : package->components()) {
        for (const EventMsg* msg : comp->eventsRange()) {
            _output->append("#include \"photongen/groundcontrol/_events_/");
            _output->appendWithFirstUpper(comp->name());
            _output->append("_");
            _output->appendWithFirstUpper(msg->name());
            _output->append(".hpp\"\n");
        }
    }
    _output->appendEol();

    _output->append(
                    "#include <photon/core/Rc.h>\n\n"
                    "#include <decode/ast/Ast.h>\n"
                    "#include <decode/ast/Component.h>\n"
                    "#include <decode/ast/Utils.h>\n"
                    "#include <decode/ast/Type.h>\n"
                    "#include <decode/ast/AllBuiltinTypes.h>\n"
                    "#include <decode/ast/Field.h>\n"
                    "#include <decode/parser/Project.h>\n\n"
                    "namespace photongen {\n"
                    "class Validator : public photon::RefCountable {\npublic:\n"
                    "    Validator(const decode::Project* project, const decode::Device* device)\n"
                    "        : _project(project)\n"
                    "        , _device(device)\n"
                    "    {\n");

    for (const Ast* ast : package->modules()) {
        _output->append("        decode::Rc<const decode::Ast> _");
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
                    "        decode::Rc<const decode::BuiltinType> _builtinUsize = _coreAst->builtinTypes()->usizeType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinIsize = _coreAst->builtinTypes()->isizeType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinU8 = _coreAst->builtinTypes()->u8Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinU16 = _coreAst->builtinTypes()->u16Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinU32 = _coreAst->builtinTypes()->u32Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinU64 = _coreAst->builtinTypes()->u64Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinI8 = _coreAst->builtinTypes()->i8Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinI16 = _coreAst->builtinTypes()->i16Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinI32 = _coreAst->builtinTypes()->i32Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinI64 = _coreAst->builtinTypes()->i64Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinF32 = _coreAst->builtinTypes()->f32Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinF64 = _coreAst->builtinTypes()->f64Type();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinVaruint = _coreAst->builtinTypes()->varuintType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinVarint = _coreAst->builtinTypes()->varintType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinBool = _coreAst->builtinTypes()->boolType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinVoid = _coreAst->builtinTypes()->voidType();\n"
                    "        decode::Rc<const decode::BuiltinType> _builtinChar = _coreAst->builtinTypes()->charType();\n\n");

    for (const Ast* ast : package->modules()) {
        for (const Type* type : ast->typesRange()) {
            appendTypeValidator(type);
        }
    }

    for (const Ast* ast : package->modules()) {
        if (ast->component().isNone()) {
            continue;
        }
        const Component* comp = ast->component().unwrap();
        for (const Command* cmd : comp->cmdsRange()) {
            appendCmdValidator(comp, cmd);
        }
    }

    _output->append("    }\n");

    for (const Ast* ast : package->modules()) {
        if (ast->component().isNone()) {
            continue;
        }
        const Component* comp = ast->component().unwrap();
        for (const Command* cmd : comp->cmdsRange()) {
            appendCmdMethods(comp, cmd);
        }
        for (const StatusMsg* msg : comp->statusesRange()) {
            appendTmMethods(comp, msg);
        }
    }

    _output->append("private:\n"
                    "    decode::Rc<const decode::Project> _project;\n"
                    "    decode::Rc<const decode::Device> _device;\n"
                   );

    for (const Ast* ast : package->modules()) {
        _output->append("    decode::Rc<const decode::Component> _");
        _output->append(ast->moduleName());
        _output->append("Component;\n");
    }

    for (const Ast* ast : package->modules()) {
        if (ast->component().isNone()) {
            continue;
        }
        const Component* comp = ast->component().unwrap();
        for (const Command* cmd : comp->cmdsRange()) {
            _output->append("    decode::Rc<const decode::Command> ");
            appendCmdFieldName(comp, cmd);
            _output->append(";\n");
        }
        for (const StatusMsg* msg : comp->statusesRange()) {
            _output->append("    decode::Rc<const decode::StatusMsg> ");
            appendStatusFieldName(comp, msg);
            _output->append(";\n");
        }
    }

    _output->append("};\n}\n\n");
    _validatedTypes.clear();
}

void GcInterfaceGen::appendCmdFieldName(const Component* comp, const Command* cmd)
{
    _output->append("_cmd");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(cmd->name());
}

void GcInterfaceGen::appendStatusFieldName(const Component* comp, const StatusMsg* msg)
{
    _output->append("_statusMsg");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(msg->name());
}

void GcInterfaceGen::appendTmMethods(const Component* comp, const StatusMsg* msg)
{
    _output->append("    bool hasStatusMsg");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(msg->name());
    _output->append("() const\n    {\n        return ");
    appendStatusFieldName(comp, msg);
    _output->append(";\n    }\n\n");


    _output->append("    bool decodeStatusMsg");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(msg->name());
    _output->append("(");
    GcMsgGen::genStatusMsgType(comp, msg, _output);
    _output->append("* msg, bmcl::MemReader* src, photon::CoderState* state) const\n    {\n");
    //TODO: validate msgs
    //_output->append("        if(!");
    //appendStatusFieldName(comp, msg);
    //_output->append(") {\n            return false;\n        }\n");

    _output->append("        if(!_");
    _output->append(comp->moduleName());
    _output->append("Component) {\n            return false;\n        }\n");

    _output->append("        return photongenDeserialize(msg, src, state);\n"
                    "    }\n\n");
}

void GcInterfaceGen::appendCmdMethods(const Component* comp, const Command* cmd)
{
    _output->append("    bool hasCmd");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(cmd->name());
    _output->append("() const\n    {\n        return ");
    appendCmdFieldName(comp, cmd);
    _output->append(";\n    }\n\n");

    _output->append("    bool encodeCmd");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->appendWithFirstUpper(cmd->name());
    _output->append("(");

    TypeReprGen gen(_output);
    for (const Field* field : cmd->fieldsRange()) {
        gen.genGcTypeRepr(field->type(), field->name());
        _output->append(", ");
    }
    _output->append("bmcl::Buffer* dest, photon::CoderState* state) const\n    {\n");
    _output->append("        if(!");
    appendCmdFieldName(comp, cmd);
    _output->append(") {\n            return false;\n        }\n");

    _output->append("        if(!_");
    _output->append(comp->moduleName());
    _output->append("Component) {\n            return false;\n        }\n"
                    "        dest->writeVarUint(_");
    _output->append(comp->moduleName());
    _output->append("Component->number());\n");

    _output->append("        dest->writeVarUint(");
    appendCmdFieldName(comp, cmd);
    _output->append("->number());\n");

    InlineSerContext ctx;
    ctx = ctx.indent();
    //TODO: use field inspector
    InlineTypeInspector inspector(_output);
    for (const Field* field : cmd->fieldsRange()) {
        inspector.inspect<false, true>(field->type(), ctx, field->name());
    }

    _output->append("        return true;\n"
                    "    }\n\n");

    if (cmd->type()->returnValue().isSome()) {
        _output->append("    bool decodeCmdRv");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->appendWithFirstUpper(cmd->name());
        _output->append("(");

        Rc<ReferenceType> ref = new ReferenceType(ReferenceKind::Pointer, true, const_cast<Type*>(cmd->type()->returnValue().unwrap()));
        gen.genGcTypeRepr(ref.get(), "rv");
        _output->append(", bmcl::MemReader* src, photon::CoderState* state) const\n    {\n");


        _output->append("        if(!");
        appendCmdFieldName(comp, cmd);
        _output->append(") {\n            return false;\n        }\n");

        _output->append("        if(!_");
        _output->append(comp->moduleName());
        _output->append("Component) {\n            return false;\n        }\n");
        inspector.inspect<false, false>(cmd->type()->returnValue().unwrap(), ctx, "(*rv)");

        _output->append("        return true;\n"
                        "    }\n\n");
    }
}

void GcInterfaceGen::appendCmdValidator(const Component* comp, const Command* cmd)
{
    _output->append("        ");
    appendCmdFieldName(comp, cmd);
    _output->append(" = decode::findCmd(");
    _output->append("_");
    _output->append(comp->moduleName());
    _output->append("Component.get(), \"");
    _output->append(cmd->name());
    _output->append("\", ");
    _output->appendNumericValue(cmd->fieldsRange().size());
    _output->append(");\n");
    std::size_t i = 0;
    for (const Field* field : cmd->fieldsRange()) {
        appendTypeValidator(field->type());
        _output->append("        decode::expectCmdArg(&");
        appendCmdFieldName(comp, cmd);
        _output->append(", ");
        _output->appendNumericValue(i);
        _output->append(", ");
        appendTestedType(field->type());
        _output->append(".get());\n");
        i++;
    }
    auto rv = cmd->type()->returnValue();
    if (rv.isSome()) {
        appendTypeValidator(rv.unwrap());
        _output->append("        decode::expectCmdRv(&");
        appendCmdFieldName(comp, cmd);
        _output->append(", ");
        appendTestedType(rv.unwrap());
        _output->append(".get());\n");
    } else {
        _output->append("        decode::expectCmdNoRv(&");
        appendCmdFieldName(comp, cmd);
        _output->append(");\n");
    }
    _output->appendEol();
}

bool GcInterfaceGen::insertValidatedType(const Type* type)
{
    if (type->isImported()) {
        return true;
    }
    if (type->resolveFinalType()->isBuiltin()) {
        return false;
    }
    SrcBuilder nameBuilder;
    appendTestedType(type, &nameBuilder);
    auto pair = _validatedTypes.emplace(nameBuilder.view().toStdString(), type);
    return pair.second;
}

void GcInterfaceGen::appendStructValidator(const StructType* type)
{
    for (const Field* field : type->fieldsRange()) {
        appendTypeValidator(field->type());
    }
    appendNamedTypeInit(type, "Struct");
    _output->append("        decode::expectFieldNum(&_");
    _output->append(type->moduleName());
    _output->appendWithFirstUpper(type->name());
    _output->append(", ");
    _output->appendNumericValue(type->fieldsRange().size());
    _output->append(");\n");

    std::size_t i = 0;
    for (const Field* field : type->fieldsRange()) {
        _output->append("        decode::expectField(&_");
        _output->append(type->moduleName());
        _output->appendWithFirstUpper(type->name());
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
}

void GcInterfaceGen::appendEnumValidator(const EnumType* type)
{
    appendNamedTypeInit(type, "Enum");
    _output->appendEol();
}

void GcInterfaceGen::appendVariantValidator(const VariantType* type)
{
    const VariantType* v = type;
    for (const VariantField* field : v->fieldsRange()) {
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple:
            for (const Type* t : field->asTupleField()->typesRange()) {
                appendTypeValidator(t);
            }
            break;
        case VariantFieldKind::Struct:
            for (const Field* f : field->asStructField()->fieldsRange()) {
                appendTypeValidator(f->type());
            }
            break;
        }
    }
    appendNamedTypeInit(type->asVariant(), "Variant");
    _output->appendEol();
}

bool GcInterfaceGen::appendTypeValidator(const Type* type)
{
    if (!insertValidatedType(type)) {
        return false;
    }
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        break;
    case TypeKind::Reference: {
        appendTypeValidator(type->asReference()->pointee());
        _output->append("        decode::Rc<decode::ReferenceType> ");
        appendTestedType(type);
        _output->append(" = decode::tryMakeReference(");
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
        _output->append("        decode::Rc<decode::ArrayType> ");
        appendTestedType(type);
        _output->append(" = tryMakeArray(");
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
        _output->append("        decode::Rc<decode::FunctionType> ");
        appendTestedType(type);
        _output->append(" = tryMakeFunction(");
        if (f->hasReturnValue()) {
            _output->append("(decode::Type*)");
            appendTestedType(f->returnValue().unwrap());
            _output->append(".get(), ");
        } else {
            _output->append("bmcl::None, ");
        }
        if (f->selfArgument().isSome()) {
            _output->append("decode::SelfArgument::");
            switch (f->selfArgument().unwrap()) {
                case SelfArgument::Reference:
                    _output->append("Reference, ");
                    break;
                case SelfArgument::MutReference:
                    _output->append("MutReference, ");
                    break;
                case SelfArgument::Value:
                    _output->append("Value, ");
                    break;
            }
        } else {
            _output->append("bmcl::None, ");
        }
        _output->append("bmcl::ArrayView<decode::Type*>{");
        foreachList(f->argumentsRange(), [this](const Field* field) {
            _output->append("(decode::Type*)");
            appendTestedType(field->type());
            _output->append(".get()");
        }, [this](const Field* field) {
            _output->append(", ");
        });
        _output->append("});\n\n");
        break;
    }
    case TypeKind::GenericParameter:
        break;
    case TypeKind::DynArray:
        appendTypeValidator(type->asDynArray()->elementType());
        _output->append("        decode::Rc<decode::DynArrayType> ");
        appendTestedType(type);
        _output->append(" = tryMakeDynArray(");
        _output->appendNumericValue(type->asDynArray()->maxSize());
        _output->append(", (decode::Type*)");
        appendTestedType(type->asDynArray()->elementType());
        _output->append(".get());\n\n");
        break;
    case TypeKind::Enum: {
        appendEnumValidator(type->asEnum());
        break;
    }
    case TypeKind::Struct: {
        appendStructValidator(type->asStruct());
        break;
    }
    case TypeKind::Variant: {
        appendVariantValidator(type->asVariant());
        break;
    }
    case TypeKind::Imported:
        appendTypeValidator(type->asImported()->link());
        break;
    case TypeKind::Alias:
        appendTypeValidator(type->asAlias()->alias());
        break;
    case TypeKind::Generic:
        appendNamedTypeInit(type->asGeneric(), "Generic");
        break;
    case TypeKind::GenericInstantiation: {
        const GenericInstantiationType* g = type->asGenericInstantiation();
        appendTypeValidator(g->genericType());
        _output->append("        decode::Rc<decode::GenericInstantiationType> ");
        appendTestedType(g);
        _output->append(" = decode::instantiateGeneric(&");
        appendTestedType(g->genericType());
        _output->append(", {");
        foreachList(g->substitutedTypesRange(), [this](const Type* type) {
            _output->append("decode::Rc<decode::Type>((decode::Type*)(");
            appendTestedType(type);
            _output->append(".get()))");
        }, [this](const Type* type) {
            _output->append(", ");
        });
        _output->append("});\n");
        break;
    }
    }
    return true;
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
        appendNamed(type->asGeneric());
        break;
    }
}

void GcInterfaceGen::appendNamedTypeInit(const NamedType* type, bmcl::StringView name)
{
    _output->append("        decode::Rc<const decode::");
    _output->append(name);
    _output->append("Type> _");
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
