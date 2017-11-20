#include "decode/generator/GcInterfaceGen.h"
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
    _output->append("class Validator {\npublic:\n"
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

    for (const Ast* ast : package->modules()) {
        for (const Type* type : ast->typesRange()) {
            appendTypeValidator(type);
        }
    }

    _output->append("    }\n");
    _output->append("private:\n"
                    "    Rc<const decode::Project> _project;\n"
                    "    Rc<const decode::Device> _device;\n");

//     for (const Ast* ast : package->modules()) {
//         _output->append("    Rc<const decode::Ast> _");
//         _output->append(ast->moduleName());
//     }

    _output->append("};\n\n");
    _validatedTypes.clear();
}

void GcInterfaceGen::appendTypeValidator(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        break;
    case TypeKind::Reference:
        break;
    case TypeKind::Array:
        break;
    case TypeKind::Function:
        break;
    case TypeKind::GenericParameter:
        break;
    case TypeKind::DynArray:
        break;
    case TypeKind::Enum: {
        auto pair = _validatedTypes.emplace(type);
        if (!pair.second) {
            break;
        }
        appendNamedTypeInit(type->asEnum(), "Enum");
        _output->appendEol();
        break;
    }
    case TypeKind::Struct: {
        auto pair = _validatedTypes.emplace(type);
        if (!pair.second) {
            break;
        }
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
            appendTestedType(type->asStruct()->moduleInfo(), field->type());
            _output->append(");\n");
            i++;
        }
        _output->appendEol();
        break;
    }
    case TypeKind::Variant: {
        auto pair = _validatedTypes.emplace(type);
        if (!pair.second) {
            break;
        }
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

void GcInterfaceGen::appendTestedType(const ModuleInfo* info, const Type* type)
{
    auto appendNamed = [this, info](bmcl::StringView name) {
        _output->append('_');
        _output->append(info->moduleName());
        _output->appendWithFirstUpper(name);
        _output->append(".get()");
    };
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        _output->append("_");
        _output->append(info->moduleName());
        _output->append("Ast->");
        _output->append(type->asBuiltin()->renderedTypeName(type->asBuiltin()->builtinTypeKind()));
        _output->append("Type()");
        break;
    case TypeKind::Reference:
        break;
    case TypeKind::Array:
        break;
    case TypeKind::Function:
        break;
    case TypeKind::GenericParameter:
        break;
    case TypeKind::DynArray:
        break;
    case TypeKind::Enum:
        appendNamed(type->asEnum()->name());
        break;
    case TypeKind::Struct:
        appendNamed(type->asStruct()->name());
        break;
    case TypeKind::Variant:
        appendNamed(type->asVariant()->name());
        break;
    case TypeKind::Imported:
        appendTestedType(info, type->asImported()->link());
        break;
    case TypeKind::Alias:
        appendTestedType(info, type->asAlias()->alias());
        break;
    case TypeKind::Generic:
        break;
    case TypeKind::GenericInstantiation:
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
