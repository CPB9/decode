#include "decode/generator/Generator.h"
#include "decode/parser/Ast.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Decl.h"
#include "decode/core/Diagnostics.h"

#include <unordered_set>
#include <iostream>

namespace decode {

Generator::Generator(const Rc<Diagnostics>& diag)
    : _diag(diag)
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _outPath = path.toStdString();
}

template <typename F>
void traverseType(const Type* type, F&& visitor, std::size_t depth = SIZE_MAX)
{
    if (depth == 0) {
        return;
    }
    if (!visitor(type)) {
        return;
    }
    depth--;

    switch (type->typeKind()) {
    case TypeKind::Builtin:
        break;
    case TypeKind::Array: {
        const ArrayType* array = static_cast<const ArrayType*>(type);
        traverseType(array->elementType().get(), std::forward<F>(visitor), depth);
        break;
    }
    case TypeKind::Enum:
        break;
    case TypeKind::Component:
        //TODO: traverse component
        break;
    case TypeKind::Resolved: {
        const ResolvedType* resolvedType = static_cast<const ResolvedType*>(type);
        traverseType(resolvedType->resolvedType().get(), std::forward<F>(visitor), depth);
        break;
    }
    case TypeKind::Imported:
        break;
    case TypeKind::Reference: {
        const ReferenceType* ref = static_cast<const ReferenceType*>(type);
        traverseType(ref->pointee().get(), std::forward<F>(visitor), depth);
        break;
    }
    case TypeKind::Struct: {
        Rc<FieldList> fieldList = static_cast<const Record*>(type)->fields();
        for (const Rc<Field>& field : fieldList->fields()) {
            traverseType(field->type().get(), std::forward<F>(visitor), depth);
        }
        break;
    }
    case TypeKind::Variant: {
        const Variant* variant = static_cast<const Variant*>(type);
        for (const Rc<VariantField>& field : variant->fields()) {
            switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                const TupleVariantField* tupleField = static_cast<const TupleVariantField*>(field.get());
                for (const Rc<Type>& type : tupleField->types()) {
                    traverseType(type.get(), std::forward<F>(visitor), depth);
                }
                break;
            }
            case VariantFieldKind::Struct: {
                Rc<FieldList> fieldList = static_cast<const StructVariantField*>(field.get())->fields();
                for (const Rc<Field>& field : fieldList->fields()) {
                    traverseType(field->type().get(), std::forward<F>(visitor), depth);
                }
                break;
            }
            default:
                assert(false);
            }
        }
        break;
    }
    default:
        assert(false);
    }
}

bool Generator::generateFromAst(const Rc<Ast>& ast)
{
    _ast = ast;

    for (const auto& it : ast->typeMap()) {
        const Type* type = it.second.get();
        _output.clear();

        switch (type->typeKind()) {
        case TypeKind::Builtin:
        case TypeKind::Array:
        case TypeKind::Reference:
        case TypeKind::Imported:
        case TypeKind::Resolved:
            break;
        case TypeKind::Struct:
            startIncludeGuard(type);
            writeIncludesAndFwdsForType(type);
            writeStruct(static_cast<const StructDecl*>(type));
            endIncludeGuard(type);
            break;
        case TypeKind::Variant:
            startIncludeGuard(type);
            writeIncludesAndFwdsForType(type);
            writeVariant(static_cast<const Variant*>(type));
            endIncludeGuard(type);
            break;
        case TypeKind::Enum:
            startIncludeGuard(type);
            writeIncludesAndFwdsForType(type);
            writeEnum(static_cast<const Enum*>(type));
            endIncludeGuard(type);
            break;
        case TypeKind::Component:
            break;
        default:
            assert(false);
        }

        if (!_output.empty()) {
            std::cout << _output;
            std::cout << "--------------------------------------" << std::endl;
        }
    }

    _ast = nullptr;
    return true;
}

static const char* builtinToC(BuiltinTypeKind kind)
{
    const char* varuintType = "uint64_t";
    switch (kind) {
    case BuiltinTypeKind::USize:
        return "size_t";
    case BuiltinTypeKind::ISize:
        return "ptrdiff_t";
    case BuiltinTypeKind::Varuint:
        return varuintType;
    case BuiltinTypeKind::Varint:
        return varuintType;
    case BuiltinTypeKind::U8:
        return "uint8_t";
    case BuiltinTypeKind::I8:
        return "int8_t";
    case BuiltinTypeKind::U16:
        return "uint16_t";
    case BuiltinTypeKind::I16:
        return "int16_t";
    case BuiltinTypeKind::U32:
        return "uint32_t";
    case BuiltinTypeKind::I32:
        return "int32_t";
    case BuiltinTypeKind::U64:
        return "uint64_t";
    case BuiltinTypeKind::I64:
        return "int64_t";
    case BuiltinTypeKind::Bool:
        return "bool";
    case BuiltinTypeKind::Unknown:
        //FIXME: report error
        assert(false);
        return nullptr;
    }

    return nullptr;
}

static const char* builtinToC(const Type* type)
{
    return builtinToC(static_cast<const BuiltinType*>(type)->builtinTypeKind());
}

static const char* builtinToName(BuiltinTypeKind kind)
{
    switch (kind) {
    case BuiltinTypeKind::USize:
        return "USize";
    case BuiltinTypeKind::ISize:
        return "ISize";
    case BuiltinTypeKind::Varuint:
        return "Varuint";
    case BuiltinTypeKind::Varint:
        return "Varint";
    case BuiltinTypeKind::U8:
        return "U8";
    case BuiltinTypeKind::I8:
        return "I8";
    case BuiltinTypeKind::U16:
        return "U16";
    case BuiltinTypeKind::I16:
        return "I16";
    case BuiltinTypeKind::U32:
        return "U32";
    case BuiltinTypeKind::I32:
        return "I32";
    case BuiltinTypeKind::U64:
        return "U64";
    case BuiltinTypeKind::I64:
        return "I64";
    case BuiltinTypeKind::Bool:
        return "Bool";
    case BuiltinTypeKind::Unknown:
        //FIXME: report error
        assert(false);
        return nullptr;
    }

    return nullptr;
}

static const char* builtinToName(const Type* type)
{
    return builtinToName(static_cast<const BuiltinType*>(type)->builtinTypeKind());
}

std::string genSliceName(const Type* topLevelType)
{
    std::string typeName;
    traverseType(topLevelType, [&typeName, topLevelType](const Type* visitedType) {
        switch (visitedType->typeKind()) {
        case TypeKind::Builtin:
            typeName.append(builtinToName(visitedType));
            return false;
        case TypeKind::Array:
            typeName.append("ArrOf");
            return true;
        case TypeKind::Reference: {
            const ReferenceType* ref = static_cast<const ReferenceType*>(visitedType);
            if (ref->isMutable()) {
                typeName.append("Mut");
            }
            switch (ref->referenceKind()) {
            case ReferenceKind::Pointer:
                typeName.append("PtrTo");
                break;
            case ReferenceKind::Reference:
                typeName.append("RefTo");
                break;
            case ReferenceKind::Slice:
                typeName.append("SliceOf");
                break;
            }
            return true;
        }
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
            //TODO: report error
            return false;
        case TypeKind::Imported:
        case TypeKind::Resolved: {
            std::string modName = visitedType->moduleInfo()->moduleName().toStdString();
            if (!modName.empty()) {
                modName.front() = std::toupper(modName.front());
                typeName.append(modName);
            }
            typeName.append(visitedType->name().toStdString());
            return false;
        }
        default:
            assert(false);
        }
    });
    return typeName;
}

void Generator::genTypeRepr(const Type* topLevelType, bmcl::StringView fieldName)
{
    bool hasPrefix = false;
    std::string typeName;
    std::string pointers;
    std::string arrayIndices;
    traverseType(topLevelType, [&](const Type* visitedType) {
        switch (visitedType->typeKind()) {
        case TypeKind::Builtin:
            typeName = builtinToC(visitedType);
            return false;
        case TypeKind::Array: {
            const ArrayType* arr = static_cast<const ArrayType*>(visitedType);
            arrayIndices.push_back('[');
            arrayIndices.append(std::to_string(arr->elementCount()));
            arrayIndices.push_back(']');
            return true;
        }
        case TypeKind::Reference: {
            const ReferenceType* ref = static_cast<const ReferenceType*>(visitedType);
            switch (ref->referenceKind()) {
            case ReferenceKind::Pointer:
            case ReferenceKind::Reference:
                if (ref->isMutable()) {
                    pointers.insert(0, " *");
                } else {
                    pointers.insert(0, " *const");
                }
                return true;
            case ReferenceKind::Slice:
                hasPrefix = true;
                typeName = genSliceName(ref);
                return false;
            }
        }
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
            //TODO: report error
            return false;
        case TypeKind::Imported:
        case TypeKind::Resolved:
            hasPrefix = true;
            typeName = visitedType->name().toStdString();
            return false;
        default:
            assert(false);
        }
    });
    if (hasPrefix) {
        writeModPrefix();
    }
    _output.append(typeName);
    if (!pointers.empty()) {
        _output.push_back(' ');
        _output.append(pointers);
    }
    _output.push_back(' ');
    _output.append(fieldName.begin(), fieldName.end());
    if (!arrayIndices.empty()) {
        _output.push_back(' ');
        _output.append(arrayIndices);
    }
}

void Generator::writeStruct(const FieldList* fields, bmcl::StringView name)
{
    writeTagHeader("struct");

    for (const Rc<Field>& field : fields->fields()) {
        _output.append("    ");
        genTypeRepr(field->type().get(), field->name());
        _output.append(";\n");
    }

    writeTagFooter(name);
}

void Generator::writeStruct(const StructDecl* type)
{
    writeStruct(type->fields().get(), type->name());
}

void Generator::writeTagHeader(bmcl::StringView name)
{
    _output.append("typedef ");
    _output.append(name.begin(), name.end());
    _output.append(" {\n");
}

void Generator::writeTagFooter(bmcl::StringView typeName)
{
    _output.append("} ");
    writeModPrefix();
    write(typeName);
    _output.append(";\n\n");
}

void Generator::writeEnum(const Enum* type)
{
    writeTagHeader("enum");

    for (const Rc<EnumConstant>& constant : type->constants()) {
        _output.append("    ");
        writeModPrefix();
        write(type->name());
        write("_");
        _output.append(constant->name().toStdString());
        if (constant->isUserSet()) {
            _output.append(" = ");
            _output.append(std::to_string(constant->value()));
        }
        _output.append(",\n");
    }

    writeTagFooter(type->name());
}

void Generator::write(bmcl::StringView value)
{
    _output.append(value.begin(), value.end());
}

void Generator::writeWithFirstUpper(bmcl::StringView value)
{
    write(value);
    std::size_t i = _output.size() - value.size();
    _output[i] = std::toupper(_output[i]);
}

void Generator::writeModPrefix()
{
    _output.append("Photon");
    writeWithFirstUpper(_ast->moduleInfo()->moduleName());
}

void Generator::writeVariant(const Variant* type)
{
    std::vector<bmcl::StringView> fieldNames;

    writeTagHeader("enum");

    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    ");
        writeModPrefix();
        write(type->name());
        _output.append("Type_");
        write(field->name());
        _output.append(",\n");
    }

    _output.append("} ");
    writeModPrefix();
    write(type->name());
    write("Type;\n");

    for (const Rc<VariantField>& field : type->fields()) {
        switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple:
                break;
            case VariantFieldKind::Struct: {
                const FieldList* fields = static_cast<const StructVariantField*>(field.get())->fields().get();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                writeStruct(fields, name);
                break;
            }
        }
    }

    writeTagHeader("struct");
    _output.append("    union {\n");

    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("        Photon");
        writeModPrefix();
        write(field->name());
        write(type->name());
        _output.append(" ");
        std::string a = field->name().toStdString();
        a.front() = std::tolower(a.front());
        _output.append(a);
        write(type->name());
        _output.append(",\n");
    }

    _output.append("    } data;\n");
    _output.append("    ");
    writeModPrefix();
    write(type->name());
    write("Type");
    _output.append(" type;\n");

    writeTagFooter(type->name());
}

void Generator::writeIncludesAndFwdsForType(const Type* topLevelType)
{
    bool needIncludeBuiltinHeader = false;
    std::unordered_set<std::string> includePaths;
    traverseType(topLevelType, [&](const Type* visitedType) {
        if (visitedType == topLevelType) {
            return true;
        }
        switch (visitedType->typeKind()) {
        case TypeKind::Builtin:
            needIncludeBuiltinHeader = true;
            return false;
        case TypeKind::Array:
            return true;
        case TypeKind::Reference:
            return true;
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
            return false;
        case TypeKind::Imported:
        case TypeKind::Resolved: {
            std::string path = visitedType->moduleInfo()->moduleName().toStdString();
            path.push_back('/');
            path.append(visitedType->name().begin(), visitedType->name().end());
            includePaths.insert(std::move(path));
            return false;
        }
        default:
            assert(false);
        }
    });
    _output.push_back('\n');
    for (const std::string& path : includePaths) {
        _output.append("#include \"photon/");
        _output.append(path);
        _output.append(".h\"");
        _output.push_back('\n');
    }
}

void Generator::startIncludeGuard(const Type* type)
{
    auto writeGuardMacro = [this, type]() {
        _output.append("__PHOTON_");
        _output.append(_ast->moduleInfo()->moduleName().toUpper());
        _output.push_back('_');
        _output.append(type->name().toUpper()); //FIXME
        _output.append("__\n");
    };
    _output.append("#ifndef ");
    writeGuardMacro();
    _output.append("#define ");
    writeGuardMacro();
}

void Generator::endIncludeGuard(const Type* type)
{
    _output.append("#endif\n");
}

void Generator::writeEol()
{
    _output.push_back('\n');
}
}
