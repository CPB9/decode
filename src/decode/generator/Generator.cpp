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
    , _target(new Target(4)) //FIXME
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath = path.toStdString();
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
    case TypeKind::Function: {
        const Function* fn = static_cast<const Function*>(type);
        if (fn->returnValue().isSome()) {
            traverseType(fn->returnValue().unwrap().get(), std::forward<F>(visitor), depth);
        }
        for (const Rc<Field>& field : fn->arguments()) {
            traverseType(field->type().get(), std::forward<F>(visitor), depth);
        }
        break;
    }
    case TypeKind::FnPointer: {
        const FnPointer* fn = static_cast<const FnPointer*>(type);
        if (fn->returnValue().isSome()) {
            traverseType(fn->returnValue().unwrap().get(), std::forward<F>(visitor), depth);
        }
        for (const Rc<Type>& arg : fn->arguments()) {
            traverseType(arg.get(), std::forward<F>(visitor), depth);
        }
        break;
    }
    case TypeKind::Builtin:
        break;
    case TypeKind::Array: {
        const ArrayType* array = static_cast<const ArrayType*>(type);
        traverseType(array->elementType().get(), std::forward<F>(visitor), depth);
        break;
    }
    case TypeKind::Enum:
        break;
    case TypeKind::Component: {
        const Component* comp = static_cast<const Component*>(type);
        for (const Rc<Field>& field : comp->parameters()->fields()) {
            traverseType(field->type().get(), std::forward<F>(visitor), depth);
        }
        for (const Rc<Function>& fn : comp->commands()->functions()) {
            traverseType(fn.get(), std::forward<F>(visitor), depth);
        }
        break;
    }
    case TypeKind::Unresolved:
        //TODO: report ICE
        break;
    case TypeKind::Reference: {
        const ReferenceType* ref = static_cast<const ReferenceType*>(type);
        traverseType(ref->pointee().get(), std::forward<F>(visitor), depth);
        break;
    }
    case TypeKind::Slice: {
        const SliceType* ref = static_cast<const SliceType*>(type);
        traverseType(ref->elementType().get(), std::forward<F>(visitor), depth);
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
            }
        }
        break;
    }
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


void Generator::genHeader(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Reference:
    case TypeKind::Unresolved:
    case TypeKind::Function:
    case TypeKind::FnPointer:
        break;
    case TypeKind::Slice:
        break;
    case TypeKind::Struct:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeStruct(static_cast<const StructDecl*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    case TypeKind::Variant:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeVariant(static_cast<const Variant*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    case TypeKind::Enum:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        writeEnum(static_cast<const Enum*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    case TypeKind::Component:
        startIncludeGuard(type);
        writeIncludesAndFwdsForType(type);
        writeCommonIncludePaths(type);
        //writeStruct(static_cast<const Component*>(type));
        writeImplBlockIncludes(type);
        writeImplFunctionPrototypes(type);
        //writeSerializerFuncPrototypes(type);
        endIncludeGuard(type);
        break;
    }
}

void Generator::writeCommonIncludePaths(const Type* type)
{
    _output.append("#include <stdbool.h>\n");
    _output.append("#include <stddef.h>\n");
    _output.append("#include <stdint.h>\n\n");
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
    case TypeKind::Unresolved:
    case TypeKind::Function:
    case TypeKind::FnPointer:
        break;
    case TypeKind::Struct:
        writeIncludes(type);
        _output.appendEol();
        writeStructDeserizalizer(static_cast<const StructDecl*>(type));
        _output.appendEol();
        writeStructSerializer(static_cast<const StructDecl*>(type));
        break;
    case TypeKind::Variant:
        writeIncludes(type);
        _output.appendEol();
        writeVariantDeserizalizer(static_cast<const Variant*>(type));
        _output.appendEol();
        writeVariantSerializer(static_cast<const Variant*>(type));
        break;
    case TypeKind::Enum:
        writeIncludes(type);
        _output.appendEol();
        writeEnumDeserizalizer(static_cast<const Enum*>(type));
        _output.appendEol();
        writeEnumSerializer(static_cast<const Enum*>(type));
        break;
    case TypeKind::Component:
        break;
    }
    _output.appendEol();
}

void Generator::writeReadableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("if (PhotonReader_ReadableSize(src) < ");
    _output.appendNumericValue(size);
    _output.append(") {\n");
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("    return PhotonError_NotEnoughData;\n");
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("}\n");
}

void Generator::writeInlineBuiltinTypeDeserializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    auto genSizedDeser = [this, ctx, &argNameGen](std::size_t size, bmcl::StringView suffix) {
        writeReadableSizeCheck(ctx, size);
        _output.appendSeveral(ctx.indentLevel, "    ");
        argNameGen();
        _output.append(" = PhotonReader_Read");
        _output.append(suffix);
        _output.append("(src);\n");
    };
    auto genVarDeser = [this, ctx, &argNameGen](bmcl::StringView suffix) {
        _output.appendSeveral(ctx.indentLevel, "    ");
        writeWithTryMacro([&, this]() {
            _output.append("PhotonReader_Read");
            _output.append(suffix);
            _output.append("(src, &");
            argNameGen();
            _output.append(")");
        });
    };
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        genSizedDeser(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        genSizedDeser(_target->pointerSize(), "ISizeLe");
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

void Generator::writeLoopHeader(const InlineSerContext& ctx, std::size_t loopSize)
{
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("for (size_t ");
    assert(ctx.indentLevel < 30);
    _output.append('a' + ctx.loopLevel);
    _output.append(" = 0; ");
    _output.append('a' + ctx.loopLevel);
    _output.append(" < ");
    _output.appendNumericValue(loopSize);
    _output.append("; ");
    _output.append('a' + ctx.loopLevel);
    _output.append("++) {\n");
}

void Generator::writeInlineArrayTypeDeserializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    writeLoopHeader(ctx, type->elementCount());

    writeInlineTypeDeserializer(type->elementType().get(), ctx.indent().incLoopVar(), [&, this]() {
        argNameGen();
        _output.append('[');
        _output.append('a' + ctx.loopLevel);
        _output.append(']');
    });
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("}\n");
}

void Generator::writeInlineArrayTypeSerializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    writeLoopHeader(ctx, type->elementCount());

    writeInlineTypeSerializer(type->elementType().get(), ctx.indent().incLoopVar(), [&, this]() {
        argNameGen();
        _output.append('[');
        _output.append('a' + ctx.loopLevel);
        _output.append(']');
    });
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("}\n");
}

void Generator::writeInlinePointerDeserializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    writeReadableSizeCheck(ctx, _target->pointerSize());
    _output.appendSeveral(ctx.indentLevel, "    ");
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
    case TypeKind::Unresolved:
        //TODO: report ICE
        break;
    case TypeKind::Function:
        assert(false && "Not implemented");
        break;
    case TypeKind::FnPointer:
        writeInlinePointerDeserializer(type, ctx, argNameGen);
        break;
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Component:
    case TypeKind::Slice:
        _output.appendSeveral(ctx.indentLevel, "    ");
        writeWithTryMacro([&, this, type]() {
            genTypeRepr(type);
            _output.append("_Deserialize(&");
            argNameGen();
            _output.append(", src)");
        });
        break;
    default:
        assert(false);
    }
}

void Generator::writeInlineBuiltinTypeSerializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    auto genSizedSer = [this, ctx, &argNameGen](std::size_t size, bmcl::StringView suffix) {
        writeWritableSizeCheck(ctx, size);
        _output.appendSeveral(ctx.indentLevel, "    ");
        _output.append("PhotonWriter_Write");
        _output.append(suffix);
        _output.append("(dest, ");
        argNameGen();
        _output.append(");\n");
    };
    auto genVarSer = [this, ctx, &argNameGen](bmcl::StringView suffix) {
        _output.appendSeveral(ctx.indentLevel, "    ");
        writeWithTryMacro([&, this]() {
            _output.append("PhotonWriter_Write");
            _output.append(suffix);
            _output.append("(dest, ");
            argNameGen();
            _output.append(")");
        });
    };
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        genSizedSer(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        genSizedSer(_target->pointerSize(), "ISizeLe");
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

void Generator::writeWritableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("if (PhotonWriter_WritableSize(dest) < ");
    _output.appendNumericValue(size);
    _output.append(") {\n");
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("    return PhotonError_NotEnoughSpace;\n");
    _output.appendSeveral(ctx.indentLevel, "    ");
    _output.append("}\n");
}

void Generator::writeInlinePointerSerializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen)
{
    writeWritableSizeCheck(ctx, _target->pointerSize());
    _output.appendSeveral(ctx.indentLevel, "    ");
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
    case TypeKind::Unresolved:
        //TODO: report ICE
        break;
    case TypeKind::Function:
        assert(false && "Not implemented");
        break;
    case TypeKind::FnPointer:
        writeInlinePointerSerializer(type, ctx, argNameGen);
        break;
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Component:
    case TypeKind::Slice:
        _output.appendSeveral(ctx.indentLevel, "    ");
        writeWithTryMacro([&, this, type]() {
            genTypeRepr(type);
            _output.append("_Serialize(&");
            argNameGen();
            _output.append(", dest)");
        });
        break;
    default:
        assert(false);
    }
}

void Generator::writeStructDeserizalizer(const StructDecl* type)
{
    InlineSerContext ctx;
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");

    for (const Rc<Field>& field : type->fields()->fields()) {
        writeInlineTypeDeserializer(field->type().get(), ctx, [this, field]() {
            _output.append("self->");
            _output.append(field->name());
        });
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeStructSerializer(const StructDecl* type)
{
    InlineSerContext ctx;
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");

    for (const Rc<Field>& field : type->fields()->fields()) {
        writeInlineTypeSerializer(field->type().get(), ctx, [this, field]() {
            _output.append("self->");
            _output.append(field->name());
        });
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeWithTryMacro(const Gen& func)
{
    _output.append("PHOTON_TRY(");
    func();
    _output.append(");\n");
}

void Generator::writeVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix)
{
    _output.append(prefix);
    _output.append(typeName);
    _output.append(' ');
    _output.appendWithFirstLower(varName);
    _output.append(";\n");
}

void Generator::writeVariantDeserizalizer(const Variant* type)
{
    writeDeserializerFuncDecl(type);
     _output.append("\n{\n");
    _output.append("    ");
    writeVarDecl("int64_t", "value");
    _output.append("    ");
    writeWithTryMacro([this, type]() {
        _output.append("PhotonReader_ReadVarint(src, &");
        _output.append("value");
        _output.append(")");
    });

    _output.append("    switch(value) {\n");
    std::size_t i = 0;
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        _output.appendNumericValue(i);
        i++;
        _output.append(": {\n");
        _output.append("        self->type = ");
        writeModPrefix();
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
            for (const Rc<Field>& f : varField->fields()->fields()) {
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

void Generator::writeVariantSerializer(const Variant* type)
{
    writeSerializerFuncDecl(type);
     _output.append("\n{\n");
    _output.append("    ");
    writeWithTryMacro([this, type]() {
        _output.append("PhotonWriter_WriteVaruint(dest, (uint64_t)self->type)");
    });

    _output.append("    switch(self->type) {\n");
    std::size_t i = 0;
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        writeModPrefix();
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
            for (const Rc<Field>& f : varField->fields()->fields()) {
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

void Generator::writeEnumDeserizalizer(const Enum* type)
{
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");
    _output.append("    ");
    writeVarDecl("int64_t", "value");
    _output.append("    ");
    writeModPrefix();
    writeVarDecl(type->name(), "result");
    _output.append("    ");
    writeWithTryMacro([this, type]() {
        _output.append("PhotonReader_ReadVarint(src, &");
        _output.append("value");
        _output.append(")");
    });

    _output.append("    switch(value) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        _output.appendNumericValue(pair.first);
        _output.append(":\n");
        _output.append("        result = ");
        writeModPrefix();
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

void Generator::writeEnumSerializer(const Enum* type)
{
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");

    _output.append("    switch(*self) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        writeModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name());
        _output.append(":\n");
    }
    _output.append("        break;\n");
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n    ");
    writeWithTryMacro([this, type]() {
        _output.append("PhotonWriter_WriteVarint(dest, (int64_t)*self)");
    });
    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

bool Generator::generateFromAst(const Rc<Ast>& ast)
{
    _ast = ast;
    _shouldWriteModPrefix = !ast->moduleInfo()->moduleName().equals("core");

    StringBuilder photonPath;
    photonPath.append(_savePath);
    photonPath.append("/photon");
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');
    photonPath.append(_ast->moduleInfo()->moduleName());
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

        genHeader(type);
        TRY(dump(type, ".h"));

        genSource(type);
        TRY(dump(type, ".c"));
    }

    _ast = nullptr;
    return true;
}

bool Generator::needsSerializers(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Unresolved:
    case TypeKind::Function:
    case TypeKind::FnPointer:
    case TypeKind::Slice:
        return false;
    case TypeKind::Reference:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Enum:
    case TypeKind::Component:
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
    bmcl::Option<const Rc<ImplBlock>&> block = _ast->findImplBlockWithName(type->name());
    if (block.isNone()) {
        return;
    }
    for (const Rc<Function>& func : block.unwrap()->functions()) {
        writeImplFunctionPrototype(func, type->name());
    }
    if (!block.unwrap()->functions().empty()) {
        _output.append('\n');
    }
}

void Generator::writeImplFunctionPrototype(const Rc<Function>& func, bmcl::StringView typeName)
{
    if (func->returnValue().isSome()) {
        genTypeRepr(func->returnValue()->get());
        _output.append(' ');
    } else {
        _output.append("void ");
    }
    writeModPrefix();
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
            writeModPrefix();
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

static const char* builtinToC(BuiltinTypeKind kind)
{
    switch (kind) {
    case BuiltinTypeKind::USize:
        return "size_t";
    case BuiltinTypeKind::ISize:
        return "ptrdiff_t";
    case BuiltinTypeKind::Varuint:
        return "uint64_t";
    case BuiltinTypeKind::Varint:
        return "int64_t";
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
    case BuiltinTypeKind::Void:
        return "void";
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
    case BuiltinTypeKind::Void:
        return "Void";
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
            }
            return true;
        }
        case TypeKind::Slice:
            typeName.append("SliceOf");
            return true;
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
        case TypeKind::Function:
        case TypeKind::FnPointer:
            //TODO: report error
            return false;
        case TypeKind::Unresolved: {
            std::string modName = visitedType->moduleName().toStdString();
            if (!modName.empty()) {
                modName.front() = std::toupper(modName.front());
                typeName.append(modName);
            }
            typeName.append(visitedType->name().toStdString());
            return false;
        }
        }
    });
    return typeName;
}

void Generator::genFnPointerTypeRepr(const FnPointer* topLevelType, bmcl::StringView fieldName)
{
    std::vector<const FnPointer*> fnStack;
    const FnPointer* current = topLevelType;
    fnStack.push_back(current);
    while (true) {
        if (current->returnValue().isSome()) {
            if (current->returnValue().unwrap()->typeKind() == TypeKind::FnPointer) {
                current = static_cast<const FnPointer*>(current->returnValue()->get());
                fnStack.push_back(current);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    if (fnStack.back()->returnValue().isSome()) {
        genTypeRepr(fnStack.back()->returnValue().unwrap().get());
    } else {
        _output.append("void");
    }
    _output.append(" ");
    for (std::size_t i = 0; i < fnStack.size(); i++) {
        _output.append("(*");
    }
    _output.append(fieldName);

    BMCL_DEBUG() << fnStack.size();
    _output.append(")(");

    auto appendParameters = [this](const FnPointer* t) {
        if (t->arguments().size() > 0) {
            for (auto jt = t->arguments().cbegin(); jt < (t->arguments().cend() - 1); jt++) {
                genTypeRepr(jt->get());
                _output.append(", ");
            }
            genTypeRepr(t->arguments().back().get());
        }
    };

    for (auto it = fnStack.begin(); it < (fnStack.end() - 1); it++) {
        appendParameters(*it);
        _output.append("))(");
    }
    appendParameters(fnStack.back());
    _output.append(")");
}

void Generator::genTypeRepr(const Type* topLevelType, bmcl::StringView fieldName)
{
    if (topLevelType->typeKind() == TypeKind::FnPointer) {
        genFnPointerTypeRepr(static_cast<const FnPointer*>(topLevelType), fieldName);
        return;
    }
    bool hasPrefix = false;
    std::string typeName;
    std::deque<bool> pointers; // isConst
    std::string arrayIndices;
    traverseType(topLevelType, [&](const Type* visitedType) {
        switch (visitedType->typeKind()) {
        case TypeKind::Function:
            return false;
        case TypeKind::FnPointer:
            genFnPointerTypeRepr(static_cast<const FnPointer*>(visitedType), fieldName);
            return false;
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
            }
        }
        case TypeKind::Slice:
            hasPrefix = true;
            typeName = genSliceName(visitedType);
            return false;
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
        case TypeKind::Unresolved:
            hasPrefix = true;
            typeName = visitedType->name().toStdString();
            return false;
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

    for (const auto& pair : type->constants()) {
        _output.append("    ");
        writeModPrefix();
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
        if (field->variantFieldKind() == VariantFieldKind::Constant) {
            continue;
        }
        _output.append("        ");
        writeModPrefix();
        _output.append(field->name());
        _output.append(type->name());
        _output.appendSpace();
        _output.appendWithFirstLower(field->name());
        _output.append(type->name());
        _output.append(";\n");
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

void Generator::writeLocalIncludePath(bmcl::StringView path)
{
    _output.append("#include \"photon/");
    _output.append(path);
    _output.append(".h\"");
    _output.appendEol();
}

void Generator::writeImplBlockIncludes(const Type* topLevelType)
{
    bmcl::Option<const Rc<ImplBlock>&> impl = _ast->findImplBlockWithName(topLevelType->name());
    std::unordered_set<std::string> dest;
    if (impl.isSome()) {
        for (const Rc<Function>& fn : impl.unwrap()->functions()) {
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
    traverseType(topLevelType, [&](const Type* visitedType) {
        if (visitedType == topLevelType) {
            return true;
        }
        switch (visitedType->typeKind()) {
        case TypeKind::Builtin:
            return false;
        case TypeKind::Array:
            return true;
        case TypeKind::Reference:
            return true;
        case TypeKind::Slice:
            return true;
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Enum:
        case TypeKind::Component:
        case TypeKind::Function:
        case TypeKind::FnPointer:
            return false;
        case TypeKind::Unresolved: {
            std::string path = visitedType->moduleName().toStdString();
            path.push_back('/');
            path.append(visitedType->name().begin(), visitedType->name().end());
            dest->insert(std::move(path));
            return false;
        }
        }
    });
}

void Generator::writeIncludes(const std::unordered_set<std::string>& src)
{
    for (const std::string& path : src) {
        writeLocalIncludePath(path);
    }

    if (!src.empty()) {
        _output.appendEol();
    }

    _output.appendEol();
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
