/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/SourceGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/IncludeCollector.h"

namespace decode {

//TODO: refact

SourceGen::SourceGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _output(output)
    , _typeReprGen(reprGen)
    , _inlineSer(reprGen, output)
    , _inlineDeser(reprGen, output)
    , _prototypeGen(reprGen, output)
{
}

SourceGen::~SourceGen()
{
}

inline bool SourceGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}


inline bool SourceGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitAliasType(const AliasType* type)
{
    (void)type;
    return false;
}

void SourceGen::appendIncludes(const NamedType* type)
{
    StringBuilder path(type->moduleName().toStdString());
    path.append('/');
    path.append(type->name());
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
    _output->appendLocalIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.result());
    _output->append(".gen.c\"\n");
}

void SourceGen::appendEnumSerializer(const EnumType* type)
{
    _output->append("    switch(self) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(c->name());
        _output->append(":\n");
    }
    _output->append("        break;\n");
    _output->append("    default:\n");
    _output->append("        PHOTON_CRITICAL(\"Failed to serialize enum\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self)");
    }, "Failed to write enum");
}

void SourceGen::appendEnumDeserializer(const EnumType* type)
{
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->appendModPrefix();
    _output->appendVarDecl(type->name(), "result");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    }, "Failed to read enum");

    _output->append("    switch(value) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(c->value());
        _output->append(":\n");
        _output->append("        result = ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(c->name());
        _output->append(";\n");
        _output->append("        break;\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_WARNING(\"Failed to deserialize enum\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");

    _output->append("    *self = result;\n");
}

static bmcl::Option<std::size_t> typeFixedSize(const Type* type)
{
    if (!type->isBuiltin()) {
        return bmcl::None;
    }
    switch (type->asBuiltin()->builtinTypeKind()) {
        //TODO: calc usize/isize types depending on target
    case BuiltinTypeKind::USize:
    case BuiltinTypeKind::ISize:
    case BuiltinTypeKind::Varint:
    case BuiltinTypeKind::Varuint:
    case BuiltinTypeKind::Void:
        return bmcl::None;
    case BuiltinTypeKind::Bool:
    case BuiltinTypeKind::Char:
    case BuiltinTypeKind::U8:
    case BuiltinTypeKind::I8:
        return 1;
    case BuiltinTypeKind::U16:
    case BuiltinTypeKind::I16:
        return 2;
    case BuiltinTypeKind::F32:
    case BuiltinTypeKind::U32:
    case BuiltinTypeKind::I32:
        return 4;
    case BuiltinTypeKind::U64:
    case BuiltinTypeKind::I64:
    case BuiltinTypeKind::F64:
        return 8;
    }
    return bmcl::None;
}

template <typename I>
void inspectStruct(const StructType* type, I* inspector, SrcBuilder* dest)
{
    InlineSerContext ctx;
    StringBuilder argName("self->");
    auto begin = type->fieldsBegin();
    auto it = begin;
    auto end = type->fieldsEnd();

    while (it != end) {
        bmcl::Option<std::size_t> totalSize;
        while (it != end) {
            bmcl::Option<std::size_t> size = typeFixedSize(it->type());
            if (size.isNone()) {
                break;
            }
            totalSize.emplace(totalSize.unwrapOr(0) + size.unwrap());
            it++;
        }
        if (totalSize.isSome()) {
            inspector->appendSizeCheck(ctx, std::to_string(totalSize.unwrap()), dest);
            for (auto jt = begin; jt < it; jt++) {
                argName.append(jt->name().begin(), jt->name().end());
                inspector->inspect(jt->type(), ctx, argName.view(), false);
                argName.resize(6);
            }

            totalSize.clear();
        } else {
            argName.append(it->name().begin(), it->name().end());
            inspector->inspect(it->type(), ctx, argName.view());
            argName.resize(6);
            it++;
            begin = it;
        }
    }
}

void SourceGen::appendStructSerializer(const StructType* type)
{
    inspectStruct(type, &_inlineSer, _output);
}

void SourceGen::appendStructDeserializer(const StructType* type)
{
    inspectStruct(type, &_inlineDeser, _output);
}

void SourceGen::appendVariantSerializer(const VariantType* type)
{
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self->type)");
    }, "Failed to write variant type");

    _output->append("    switch(self->type) {\n");
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(": {\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<const TupleVariantField*>(field);
            std::size_t j = 1;
            for (const Type* t : tupField->typesRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append("._");
                argName.appendNumericValue(j);
                _inlineSer.inspect(t, ctx, argName.view());
                argName.resize(11);
                j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append(".");
                argName.append(f->name());
                _inlineSer.inspect(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_CRITICAL(\"Failed to serialize variant\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");
}

void SourceGen::appendVariantDeserializer(const VariantType* type)
{
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    }, "Failed to read variant type");

    _output->append("    switch(value) {\n");
    std::size_t i = 0;
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(i);
        i++;
        _output->append(": {\n");
        _output->append("        self->type = ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<const TupleVariantField*>(field);
            std::size_t j = 1;
            for (const Type* t : tupField->typesRange()) {
                    argName.appendWithFirstLower(field->name());
                    argName.append(type->name());
                    argName.append("._");
                    argName.appendNumericValue(j);
                    _inlineDeser.inspect(t, ctx, argName.view());
                    argName.resize(11);
                    j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append(".");
                argName.append(f->name());
                _inlineDeser.inspect(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_WARNING(\"Failed to deserialize variant\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");
}

void SourceGen::appendDynArraySerializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->append("    if (self->size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_CRITICAL(\"Failed to serialize dynarray\");\n");
    _output->append("        return PhotonError_InvalidValue;\n    }\n");
    _output->append("    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, self->size)");
    }, "Failed to write dynarray size");
    _output->appendLoopHeader(ctx, "self->size");
    InlineSerContext lctx = ctx.indent();
    _inlineSer.inspect(type->elementType(), lctx, "self->data[a]");
    _output->append("    }\n");
}

void SourceGen::appendDynArrayDeserializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->appendIndent(1);
    _output->appendVarDecl("uint64_t", "size");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVaruint(src, &size)");
    }, "Failed to read dynarray size");
    _output->append("    if (size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_WARNING(\"Failed to deserialize dynarray\");\n");
    _output->append("        return PhotonError_InvalidValue;\n    }\n");
    _output->appendLoopHeader(ctx, "size");
    InlineSerContext lctx = ctx.indent();
    _inlineDeser.inspect(type->elementType(), lctx, "self->data[a]");
    _output->append("    }\n");
    if (type->elementType()->isBuiltinChar()) {
        _output->append("    self->data[size] = '\\0';\n");
    }
    _output->append("    self->size = size;\n");
}

template <typename T, typename F>
void SourceGen::genSource(const T* type, F&& serGen, F&& deserGen)
{
    appendIncludes(type);
    _output->appendEol();
    _prototypeGen.appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    (this->*serGen)(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
    _output->appendEol();
    _prototypeGen.appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    (this->*deserGen)(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n\n");
    _output->append("#undef _PHOTON_FNAME");
    _output->appendEol();
}

bool SourceGen::visitDynArrayType(const DynArrayType* type)
{
    StringBuilder path("_dynarray_/");
    path.append(TypeNameGen::genTypeNameAsString(type));
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
    _output->appendLocalIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.result());
    _output->append(".gen.c\"\n\n");
    _prototypeGen.appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    appendDynArraySerializer(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
    _output->appendEol();
    _prototypeGen.appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    appendDynArrayDeserializer(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n\n");
    _output->append("#undef _PHOTON_FNAME");
    _output->appendEol();
    return false;
}

bool SourceGen::visitEnumType(const EnumType* type)
{
    genSource(type, &SourceGen::appendEnumSerializer, &SourceGen::appendEnumDeserializer);
    return false;
}

bool SourceGen::visitStructType(const StructType* type)
{
    genSource(type, &SourceGen::appendStructSerializer, &SourceGen::appendStructDeserializer);
    return false;
}

bool SourceGen::visitVariantType(const VariantType* type)
{
    genSource(type, &SourceGen::appendVariantSerializer, &SourceGen::appendVariantDeserializer);
    return false;
}

void SourceGen::genTypeSource(const Type* type)
{
    traverseType(type);
}
}
