#include "decode/generator/Generator.h"
#include "decode/parser/Ast.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Decl.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"

#include <bmcl/Logging.h>

#include <unordered_set>
#include <iostream>
#include <deque>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace decode {

Generator::Generator(const Rc<Diagnostics>& diag)
    : _diag(diag)
    , _shouldWriteModPrefix(true)
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

bool Generator::makeDirectory(const char* path)
{
    int rv = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv == -1) {
        int rn = errno;
        if (rn == EEXIST) {
            return true;
        }
        //TODO: report error
        BMCL_CRITICAL() << "unable to create dir: " << path;
        return false;
    }
    return true;
}

bool Generator::saveOutput(const char* path)
{
    int fd;
    while (true) {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            //TODO: handle error
            BMCL_CRITICAL() << "unable to open file: " << path;
            return false;
        }
        break;
    }

    std::size_t size = _output.result().size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, _output.result().c_str() + total, size);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            BMCL_CRITICAL() << "unable to write file: " << path;
            //TODO: handle error
            close(fd);
            return false;
        }
        total += written;
    }

    close(fd);
    return true;
}

bool Generator::generateFromAst(const Rc<Ast>& ast)
{
    _ast = ast;
    _shouldWriteModPrefix = !ast->moduleInfo()->moduleName().equals("core");

    StringBuilder photonPath;
    photonPath.append(_outPath);
    photonPath.append("/photon");
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');
    photonPath.append(_ast->moduleInfo()->moduleName());
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');

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
            writeSerializerFuncPrototypes(type);
            endIncludeGuard(type);
            break;
        case TypeKind::Variant:
            startIncludeGuard(type);
            writeIncludesAndFwdsForType(type);
            writeVariant(static_cast<const Variant*>(type));
            writeSerializerFuncPrototypes(type);
            endIncludeGuard(type);
            break;
        case TypeKind::Enum:
            startIncludeGuard(type);
            writeEnum(static_cast<const Enum*>(type));
            writeSerializerFuncPrototypes(type);
            endIncludeGuard(type);
            break;
        case TypeKind::Component:
            break;
        default:
            assert(false);
        }

        if (!_output.result().empty()) {
            photonPath.append(type->name());
            photonPath.append(".h");
            TRY(saveOutput(photonPath.result().c_str()));
            photonPath.removeFromBack(type->name().size() + 2);
        }
    }

    _ast = nullptr;
    return true;
}

bool Generator::needsSerializers(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Imported:
    case TypeKind::Resolved:
        return false;
    case TypeKind::Reference: {
        const ReferenceType* ref = static_cast<const ReferenceType*>(type);
        if (ref->referenceKind() != ReferenceKind::Slice) {
            return false;
        }
        break;
    }
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Component:
        break;
    default:
        assert(false);
    }
    return true;
}

void Generator::writeSerializerFuncDecl(const Type* type)
{
    if (!needsSerializers(type)) {
        return;
    }
    _output.append("PhotonError ");
    genTypeRepr(type);
    _output.append("_Serialize(");
    genTypeRepr(type);
    _output.append("* src, PhotonWriter* dest)");
}

void Generator::writeDeserializerFuncDecl(const Type* type)
{
    if (!needsSerializers(type)) {
        return;
    }
    _output.append("PhotonError ");
    genTypeRepr(type);
    _output.append("_Deserialize(");
    genTypeRepr(type);
    _output.append("* dest, PhotonReader* src)");
}

void Generator::writeSerializerFuncPrototypes(const Type* type)
{
    writeSerializerFuncDecl(type);
    _output.append(";\n");
    writeDeserializerFuncDecl(type);
    _output.append(";\n\n");
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
    std::deque<bool> pointers; // isConst
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
                    pointers.push_front(false);
                } else {
                    pointers.push_front(true);
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
        case TypeKind::Imported:
        case TypeKind::Resolved:
            hasPrefix = true;
            typeName = visitedType->name().toStdString();
            return false;
        default:
            assert(false);
        }
    });

    auto writeTypeName = [&]() {
        if (hasPrefix) {
            writeModPrefix();
        }
        _output.append(typeName);
    };

    if (pointers.size() == 1) {
        if (pointers[0] == true) {
            _output.append("const ");
        }
        writeTypeName();
        _output.append('*');
    } else {
        writeTypeName();

        for (bool isConst : pointers) {
            if (isConst) {
                _output.append(" *const");
            } else {
                _output.append(" *");
            }
        }
    }
    if (!fieldName.isEmpty()) {
        _output.appendSpace();
        _output.append(fieldName.begin(), fieldName.end());
    }
    if (!arrayIndices.empty()) {
        _output.appendSpace();
        _output.append(arrayIndices);
    }
}

void Generator::writeStruct(const std::vector<Rc<Type>>& fields, bmcl::StringView name)
{
    writeTagHeader("struct");

    std::size_t i = 1;
    for (const Rc<Type>& type : fields) {
        _output.append("    ");
        genTypeRepr(type.get(), "_" + std::to_string(i));
        _output.append(";\n");
        i++;
    }

    writeTagFooter(name);
    _output.appendEol();
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
    _output.appendEol();
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
    _output.append(typeName);
    _output.append(";\n");
}

void Generator::writeEnum(const Enum* type)
{
    writeTagHeader("enum");

    for (const Rc<EnumConstant>& constant : type->constants()) {
        _output.append("    ");
        writeModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(constant->name().toStdString());
        if (constant->isUserSet()) {
            _output.append(" = ");
            _output.append(std::to_string(constant->value()));
        }
        _output.append(",\n");
    }

    writeTagFooter(type->name());
    _output.appendEol();
}

void Generator::writeModPrefix()
{
    _output.append("Photon");
    if (_shouldWriteModPrefix) {
        _output.appendWithFirstUpper(_ast->moduleInfo()->moduleName());
    }
}

void Generator::writeVariant(const Variant* type)
{
    std::vector<bmcl::StringView> fieldNames;

    writeTagHeader("enum");

    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    ");
        writeModPrefix();
        _output.append(type->name());
        _output.append("Type_");
        _output.append(field->name());
        _output.append(",\n");
    }

    _output.append("} ");
    writeModPrefix();
    _output.append(type->name());
    _output.append("Type;\n");
    _output.appendEol();

    for (const Rc<VariantField>& field : type->fields()) {
        switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                const std::vector<Rc<Type>>& types = static_cast<const TupleVariantField*>(field.get())->types();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                writeStruct(types, name);
                break;
            }
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
        _output.append("        ");
        writeModPrefix();
        _output.append(field->name());
        _output.append(type->name());
        _output.appendSpace();
        _output.appendWithFirstLower(field->name());
        _output.append(type->name());
        _output.append(",\n");
    }

    _output.append("    } data;\n");
    _output.append("    ");
    writeModPrefix();
    _output.append(type->name());
    _output.append("Type");
    _output.append(" type;\n");

    writeTagFooter(type->name());
    _output.appendEol();
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

    for (const std::string& path : includePaths) {
        _output.append("#include \"photon/");
        _output.append(path);
        _output.append(".h\"");
        _output.appendEol();
    }

    if (!includePaths.empty()) {
        _output.appendEol();
    }
}

void Generator::startIncludeGuard(const Type* type)
{
    auto writeGuardMacro = [this, type]() {
        _output.append("__PHOTON_");
        _output.append(_ast->moduleInfo()->moduleName().toUpper());
        _output.append('_');
        _output.append(type->name().toUpper()); //FIXME
        _output.append("__\n");
    };
    _output.append("#ifndef ");
    writeGuardMacro();
    _output.append("#define ");
    writeGuardMacro();
    _output.appendEol();
}

void Generator::endIncludeGuard(const Type* type)
{
    _output.append("#endif\n");
    _output.appendEol();
}
}
