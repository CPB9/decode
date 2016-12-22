#include "decode/generator/Generator.h"
#include "decode/generator/SliceNameGenerator.h"
#include "decode/generator/TypeReprGenerator.h"
#include "decode/generator/InlineSerContext.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Decl.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"

#include <bmcl/Logging.h>

#include <unordered_set>
#include <iostream>
#include <deque>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER)
# include <windows.h>
#endif

//TODO: refact

namespace decode {

Generator::Generator(const Rc<Diagnostics>& diag)
    : _diag(diag)
    , _target(new Target(4)) //FIXME
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath = path.toStdString();
}

bool Generator::makeDirectory(const char* path)
{
#if defined(__linux__)
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
#elif defined(_MSC_VER)
    bool isOk = CreateDirectory(path, NULL);
    if (!isOk) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            BMCL_CRITICAL() << "error creating dir";
            return false;
        }
    }
#endif
    return true;
}

bool Generator::saveOutput(const char* path)
{
#if defined(__linux__)
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
#elif defined(_MSC_VER)
    HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        BMCL_CRITICAL() << "error creating file";
        //TODO: report error
        return false;
    }
    DWORD bytesWritten;
    bool isOk = WriteFile(handle, _output.result().c_str(), _output.result().size(), &bytesWritten, NULL);
    if (!isOk) {
        BMCL_CRITICAL() << "error writing file";
        //TODO: report error
        return false;
    }
    assert(_output.result().size() == bytesWritten);
#endif
    return true;
}


void Generator::genHeader(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Reference:
    case TypeKind::Imported:
    case TypeKind::Function:
        break;
    case TypeKind::Slice:
        break;
    case TypeKind::Struct:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeStruct(static_cast<const StructType*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    case TypeKind::Variant:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeVariant(static_cast<const VariantType*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    case TypeKind::Enum:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeEnum(static_cast<const EnumType*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    }
}

void Generator::writeCommonIncludePaths(const Type* type)
{
    _output.appendInclude("stdbool.h");
    _output.appendInclude("stddef.h");
    _output.appendInclude("stdint.h");
    _output.appendEol();
}

void Generator::genSource(const Type* type)
{
    auto writeIncludes = [this](const Type* type) {
        std::string path = type->moduleName().toStdString();
        path.push_back('/');
        path.append(type->name().toStdString());
        writeLocalIncludePath(path);
    };

    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Reference:
    case TypeKind::Slice:
    case TypeKind::Imported:
    case TypeKind::Function:
        break;
    case TypeKind::Struct:
        writeIncludes(type);
        _output.appendEol();
        writeStructDeserizalizer(static_cast<const StructType*>(type));
        _output.appendEol();
        writeStructSerializer(static_cast<const StructType*>(type));
        break;
    case TypeKind::Variant:
        writeIncludes(type);
        _output.appendEol();
        writeVariantDeserizalizer(static_cast<const VariantType*>(type));
        _output.appendEol();
        writeVariantSerializer(static_cast<const VariantType*>(type));
        break;
    case TypeKind::Enum:
        writeIncludes(type);
        _output.appendEol();
        writeEnumDeserizalizer(static_cast<const EnumType*>(type));
        _output.appendEol();
        writeEnumSerializer(static_cast<const EnumType*>(type));
        break;
    }
    _output.appendEol();
}

void Generator::writeInlineBuiltinTypeDeserializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    auto genSizedDeser = [this, ctx, &argNameGen](std::size_t size, bmcl::StringView suffix) {
        _output.appendReadableSizeCheck(ctx, size);
        _output.appendIndent(ctx);
        argNameGen();
        _output.append(" = PhotonReader_Read");
        _output.append(suffix);
        _output.append("(src);\n");
    };
    auto genVarDeser = [this, ctx, &argNameGen](bmcl::StringView suffix) {
        _output.appendIndent(ctx);
        _output.appendWithTryMacro([&](SrcBuilder* output) {
            output->append("PhotonReader_Read");
            output->append(suffix);
            output->append("(src, &");
            argNameGen();
            output->append(")");
        });
    };
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        genSizedDeser(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        genSizedDeser(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::U8:
        genSizedDeser(1, "U8");
        break;
    case BuiltinTypeKind::I8:
        genSizedDeser(1, "U8");
        break;
    case BuiltinTypeKind::U16:
        genSizedDeser(2, "U16Le");
        break;
    case BuiltinTypeKind::I16:
        genSizedDeser(2, "U16Le");
        break;
    case BuiltinTypeKind::U32:
        genSizedDeser(4, "U32Le");
        break;
    case BuiltinTypeKind::I32:
        genSizedDeser(4, "U32Le");
        break;
    case BuiltinTypeKind::U64:
        genSizedDeser(8, "U64Le");
        break;
    case BuiltinTypeKind::I64:
        genSizedDeser(8, "U64Le");
        break;
    case BuiltinTypeKind::Bool:
        genSizedDeser(1, "U8");
        break;
    case BuiltinTypeKind::Varuint:
        genVarDeser("Varuint");
        break;
    case BuiltinTypeKind::Varint:
        genVarDeser("Varint");
        break;
    case BuiltinTypeKind::Void:
        //TODO: disallow
        assert(false);
        break;
    case BuiltinTypeKind::Unknown:
        assert(false);
        break;
    default:
        assert(false);
    }
}

void Generator::writeInlineArrayTypeDeserializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    _output.appendLoopHeader(ctx, type->elementCount());

    writeInlineTypeDeserializer(type->elementType().get(), ctx.indent().incLoopVar(), [&, this]() {
        argNameGen();
        _output.append('[');
        _output.append(ctx.currentLoopVar());
        _output.append(']');
    });
    _output.appendIndent(ctx);
    _output.append("}\n");
}

void Generator::writeInlinePointerDeserializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    _output.appendReadableSizeCheck(ctx, _target->pointerSize());
    _output.appendIndent(ctx);
    argNameGen();
    _output.append(" = (");
    genTypeRepr(type);
    _output.append(")PhotonReader_ReadPtr(src);\n");
}

void Generator::writeInlineTypeDeserializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        writeInlineBuiltinTypeDeserializer(static_cast<const BuiltinType*>(type), ctx, argNameGen);
        break;
    case TypeKind::Array:
        writeInlineArrayTypeDeserializer(static_cast<const ArrayType*>(type), ctx, argNameGen);
        break;
    case TypeKind::Reference:
        writeInlinePointerDeserializer(type, ctx, argNameGen);
        break;
    case TypeKind::Imported:
        //TODO: report ICE
        break;
    case TypeKind::Function:
        writeInlinePointerDeserializer(type, ctx, argNameGen);
        break;
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Slice:
        _output.appendIndent(ctx);
        _output.appendWithTryMacro([&, this, type](SrcBuilder* output) {
            genTypeRepr(type);
            output->append("_Deserialize(&");
            argNameGen();
            output->append(", src)");
        });
        break;
    default:
        assert(false);
    }
}

void Generator::writeInlineBuiltinTypeSerializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    auto genSizedSer = [this, ctx, &argNameGen](std::size_t size, bmcl::StringView suffix) {
        _output.appendWritableSizeCheck(ctx, size);
        _output.appendIndent(ctx);
        _output.append("PhotonWriter_Write");
        _output.append(suffix);
        _output.append("(dest, ");
        argNameGen();
        _output.append(");\n");
    };
    auto genVarSer = [this, ctx, &argNameGen](bmcl::StringView suffix) {
        _output.appendIndent(ctx);
        _output.appendWithTryMacro([&](SrcBuilder* output) {
            output->append("PhotonWriter_Write");
            output->append(suffix);
            output->append("(dest, ");
            argNameGen();
            output->append(")");
        });
    };
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        genSizedSer(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        genSizedSer(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::U8:
        genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::I8:
        genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::U16:
        genSizedSer(2, "U16Le");
        break;
    case BuiltinTypeKind::I16:
        genSizedSer(2, "U16Le");
        break;
    case BuiltinTypeKind::U32:
        genSizedSer(4, "U32Le");
        break;
    case BuiltinTypeKind::I32:
        genSizedSer(4, "U32Le");
        break;
    case BuiltinTypeKind::U64:
        genSizedSer(8, "U64Le");
        break;
    case BuiltinTypeKind::I64:
        genSizedSer(8, "U64Le");
        break;
    case BuiltinTypeKind::Bool:
        genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::Varuint:
        genVarSer("Varuint");
        break;
    case BuiltinTypeKind::Varint:
        genVarSer("Varint");
        break;
    case BuiltinTypeKind::Void:
        //TODO: disallow
        assert(false);
        break;
    case BuiltinTypeKind::Unknown:
        assert(false);
        break;
    default:
        assert(false);
    }
}

void Generator::writeInlineArrayTypeSerializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    _output.appendLoopHeader(ctx, type->elementCount());

    writeInlineTypeSerializer(type->elementType().get(), ctx.indent().incLoopVar(), [&, this]() {
        argNameGen();
        _output.append('[');
        _output.append(ctx.currentLoopVar());
        _output.append(']');
    });
    _output.appendIndent(ctx);
    _output.append("}\n");
}

void Generator::writeInlinePointerSerializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    _output.appendWritableSizeCheck(ctx, _target->pointerSize());
    _output.appendIndent(ctx);
    _output.append("PhotonWriter_WritePtr(dest, ");
    argNameGen();
    _output.append(");\n");
}

void Generator::writeInlineTypeSerializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        writeInlineBuiltinTypeSerializer(static_cast<const BuiltinType*>(type), ctx, argNameGen);
        break;
    case TypeKind::Array:
        writeInlineArrayTypeSerializer(static_cast<const ArrayType*>(type), ctx, argNameGen);
        break;
    case TypeKind::Reference:
        writeInlinePointerSerializer(type, ctx, argNameGen);
        break;
    case TypeKind::Imported:
        //TODO: report ICE
        break;
    case TypeKind::Function:
        writeInlinePointerSerializer(type, ctx, argNameGen);
        break;
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Slice:
        _output.appendIndent(ctx);
        _output.appendWithTryMacro([&, this, type](SrcBuilder* output) {
            genTypeRepr(type);
            output->append("_Serialize(&");
            argNameGen();
            output->append(", dest)");
        });
        break;
    default:
        assert(false);
    }
}

void Generator::writeStructDeserizalizer(const StructType* type)
{
    InlineSerContext ctx;
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");

    for (const Rc<Field>& field : *type->fields()) {
        writeInlineTypeDeserializer(field->type().get(), ctx, [this, field]() {
            _output.append("self->");
            _output.append(field->name());
        });
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeStructSerializer(const StructType* type)
{
    InlineSerContext ctx;
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");

    for (const Rc<Field>& field : *type->fields()) {
        writeInlineTypeSerializer(field->type().get(), ctx, [this, field]() {
            _output.append("self->");
            _output.append(field->name());
        });
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeVariantDeserizalizer(const VariantType* type)
{
    writeDeserializerFuncDecl(type);
     _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendVarDecl("int64_t", "value");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output.append("    switch(value) {\n");
    std::size_t i = 0;
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        _output.appendNumericValue(i);
        i++;
        _output.append(": {\n");
        _output.append("        self->type = ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("Type");
        _output.append("_");
        _output.appendWithFirstUpper(field->name());
        _output.append(";\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<TupleVariantField*>(field.get());
            std::size_t j = 1;
            for (const Rc<Type>& t : tupField->types()) {
                writeInlineTypeDeserializer(t.get(), ctx, [&, this]() {
                    _output.append("self->data.");
                    _output.appendWithFirstLower(field->name());
                    _output.append(type->name());
                    _output.append("._");
                    _output.appendNumericValue(j);
                    j++;
                });
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<StructVariantField*>(field.get());
            for (const Rc<Field>& f : *varField->fields()) {
                writeInlineTypeDeserializer(f->type().get(), ctx, [&, this]() {
                    _output.append("self->data.");
                    _output.appendWithFirstLower(field->name());
                    _output.append(type->name());
                    _output.append(".");
                    _output.append(f->name());
                });
            }
            break;
        }
        }

        _output.append("        break;\n");
        _output.append("    }\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeVariantSerializer(const VariantType* type)
{
    writeSerializerFuncDecl(type);
     _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, (uint64_t)self->type)");
    });

    _output.append("    switch(self->type) {\n");
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("Type");
        _output.append("_");
        _output.appendWithFirstUpper(field->name());
        _output.append(": {\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<TupleVariantField*>(field.get());
            std::size_t j = 1;
            for (const Rc<Type>& t : tupField->types()) {
                writeInlineTypeSerializer(t.get(), ctx, [&, this]() {
                    _output.append("self->data.");
                    _output.appendWithFirstLower(field->name());
                    _output.append(type->name());
                    _output.append("._");
                    _output.appendNumericValue(j);
                    j++;
                });
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<StructVariantField*>(field.get());
            for (const Rc<Field>& f : *varField->fields()) {
                writeInlineTypeSerializer(f->type().get(), ctx, [&, this]() {
                    _output.append("self->data.");
                    _output.appendWithFirstLower(field->name());
                    _output.append(type->name());
                    _output.append(".");
                    _output.append(f->name());
                });
            }
            break;
        }
        }

        _output.append("        break;\n");
        _output.append("    }\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeEnumDeserizalizer(const EnumType* type)
{
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendVarDecl("int64_t", "value");
    _output.appendIndent(1);
    _output.appendModPrefix();
    _output.appendVarDecl(type->name(), "result");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output.append("    switch(value) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        _output.appendNumericValue(pair.first);
        _output.append(":\n");
        _output.append("        result = ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name());
        _output.append(";\n");
        _output.append("        break;\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    *self = result;\n");
    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeEnumSerializer(const EnumType* type)
{
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");

    _output.append("    switch(*self) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name());
        _output.append(":\n");
    }
    _output.append("        break;\n");
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n    ");
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)*self)");
    });
    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

bool Generator::generateFromPackage(const Rc<Package>& package)
{
    for (auto it : package->modules()) {
        if (!generateFromAst(it.second)) {
            return false;
        }
    }

    return true;
}

bool Generator::generateFromAst(const Rc<Ast>& ast)
{
    _currentAst = ast;
    if (!ast->moduleInfo()->moduleName().equals("core")) {
        _output.setModName(ast->moduleInfo()->moduleName());
    } else {
        _output.setModName(bmcl::StringView::empty());
    }

    StringBuilder photonPath;
    photonPath.append(_savePath);
    photonPath.append("/photon");
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');
    photonPath.append(_currentAst->moduleInfo()->moduleName());
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');

    auto dump = [this, &photonPath](const Type* type, bmcl::StringView ext) -> bool {
        if (!_output.result().empty()) {
            photonPath.append(type->name());
            photonPath.append(ext);
            TRY(saveOutput(photonPath.result().c_str()));
            photonPath.removeFromBack(type->name().size() + 2);
            _output.clear();
        }
        return true;
    };

    for (const auto& it : ast->typeMap()) {
        const Type* type = it.second.get();
        if (type->typeKind() == TypeKind::Imported) {
            return true;
        }

        genHeader(type);
        TRY(dump(type, ".h"));

        genSource(type);
        TRY(dump(type, ".c"));
    }

    _currentAst = nullptr;
    return true;
}

bool Generator::needsSerializers(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Imported:
    case TypeKind::Function:
    case TypeKind::Slice:
        return false;
    case TypeKind::Reference:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
        break;
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
    _output.append("_Serialize(const ");
    genTypeRepr(type);
    _output.append("* self, PhotonWriter* dest)");
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
    _output.append("* self, PhotonReader* src)");
}

void Generator::writeSerializerFuncPrototypes(const Type* type)
{
    writeSerializerFuncDecl(type);
    _output.append(";\n");
    writeDeserializerFuncDecl(type);
    _output.append(";\n\n");
}

void Generator::writeImplFunctionPrototypes(const Type* type)
{
    bmcl::Option<const Rc<ImplBlock>&> block = _currentAst->findImplBlockWithName(type->name());
    if (block.isNone()) {
        return;
    }
    for (const Rc<FunctionType>& func : block.unwrap()->functions()) {
        writeImplFunctionPrototype(func, type->name());
    }
    if (!block.unwrap()->functions().empty()) {
        _output.append('\n');
    }
}

void Generator::writeImplFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName)
{
    if (func->returnValue().isSome()) {
        genTypeRepr(func->returnValue()->get());
        _output.append(' ');
    } else {
        _output.append("void ");
    }
    _output.appendModPrefix();
    _output.append(typeName);
    _output.append('_');
    _output.appendWithFirstUpper(func->name());
    _output.append('(');

    if (func->selfArgument().isSome()) {
        SelfArgument self = func->selfArgument().unwrap();
        switch(self) {
        case SelfArgument::Reference:
            _output.append("const ");
        case SelfArgument::MutReference:
            _output.appendModPrefix();
            _output.append(typeName);
            _output.append("* self");
            break;
        case SelfArgument::Value:
            break;
        }
        if (!func->arguments().empty()) {
            _output.append(", ");
        }
    }

    if (func->arguments().size() > 0) {
        for (auto it = func->arguments().begin(); it < (func->arguments().end() - 1); it++) {
            const Field* field = it->get();
            genTypeRepr(field->type().get(), field->name());
            _output.append(", ");
        }
        genTypeRepr(func->arguments().back()->type().get(), func->arguments().back()->name());
    }

    _output.append(");\n");
}

void Generator::genTypeRepr(const Type* topLevelType, bmcl::StringView fieldName)
{
    TypeReprGenerator trg(&_output);
    trg.genTypeRepr(topLevelType, fieldName);
}

void Generator::writeStruct(const std::vector<Rc<Type>>& fields, bmcl::StringView name)
{
    writeTagHeader("struct");

    std::size_t i = 1;
    for (const Rc<Type>& type : fields) {
        _output.appendIndent(1);
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

    for (const Rc<Field>& field : *fields) {
        _output.appendIndent(1);
        genTypeRepr(field->type().get(), field->name());
        _output.append(";\n");
    }

    writeTagFooter(name);
    _output.appendEol();
}

void Generator::writeStruct(const StructType* type)
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
    _output.appendModPrefix();
    _output.append(typeName);
    _output.append(";\n");
}

void Generator::writeEnum(const EnumType* type)
{
    writeTagHeader("enum");

    for (const auto& pair : type->constants()) {
        _output.appendIndent(1);
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name().toStdString());
        if (pair.second->isUserSet()) {
            _output.append(" = ");
            _output.append(std::to_string(pair.second->value()));
        }
        _output.append(",\n");
    }

    writeTagFooter(type->name());
    _output.appendEol();
}

void Generator::writeVariant(const VariantType* type)
{
    std::vector<bmcl::StringView> fieldNames;

    writeTagHeader("enum");

    for (const Rc<VariantField>& field : type->fields()) {
        _output.appendIndent(1);
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("Type_");
        _output.append(field->name());
        _output.append(",\n");
    }

    _output.append("} ");
    _output.appendModPrefix();
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
        if (field->variantFieldKind() == VariantFieldKind::Constant) {
            continue;
        }
        _output.append("        ");
        _output.appendModPrefix();
        _output.append(field->name());
        _output.append(type->name());
        _output.appendSpace();
        _output.appendWithFirstLower(field->name());
        _output.append(type->name());
        _output.append(";\n");
    }

    _output.append("    } data;\n");
    _output.appendIndent(1);
    _output.appendModPrefix();
    _output.append(type->name());
    _output.append("Type");
    _output.append(" type;\n");

    writeTagFooter(type->name());
    _output.appendEol();
}

void Generator::writeLocalIncludePath(bmcl::StringView path)
{
    _output.append("#include \"photon/");
    _output.append(path);
    _output.append(".h\"");
    _output.appendEol();
}

void Generator::writeImplBlockIncludes(const Type* topLevelType)
{
    bmcl::Option<const Rc<ImplBlock>&> impl = _currentAst->findImplBlockWithName(topLevelType->name());
    std::unordered_set<std::string> dest;
    if (impl.isSome()) {
        for (const Rc<FunctionType>& fn : impl.unwrap()->functions()) {
            collectIncludesAndFwdsForType(fn.get(), &dest);
        }
    }
    dest.insert("core/Reader");
    dest.insert("core/Writer");
    dest.insert("core/Error");
    writeIncludes(dest);
}

void Generator::collectIncludesAndFwdsForType(const Type* topLevelType, std::unordered_set<std::string>* dest)
{
    IncludeCollector c;
    c.collectIncludesAndFwdsForType(topLevelType, dest);
}

void Generator::writeIncludes(const std::unordered_set<std::string>& src)
{
    for (const std::string& path : src) {
        writeLocalIncludePath(path);
    }

    if (!src.empty()) {
        _output.appendEol();
    }
}

void Generator::writeIncludesAndFwdsForType(const Type* topLevelType)
{
    std::unordered_set<std::string> includePaths;
    collectIncludesAndFwdsForType(topLevelType, &includePaths);
    writeIncludes(includePaths);
}

void Generator::startIncludeGuard(const Type* type)
{
    auto writeGuardMacro = [this, type]() {
        _output.append("__PHOTON_");
        _output.append(_currentAst->moduleInfo()->moduleName().toUpper());
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
