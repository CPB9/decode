/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/OnboardTypeSourceGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/InlineFieldInspector.h"

namespace decode {

//TODO: refact

OnboardTypeSourceGen::OnboardTypeSourceGen(SrcBuilder* output)
    : _output(output)
    , _inlineInspector(output)
    , _prototypeGen(output)
{
}

OnboardTypeSourceGen::~OnboardTypeSourceGen()
{
}

void OnboardTypeSourceGen::appendIncludes(bmcl::StringView modName)
{
    StringBuilder path(modName.toStdString());
    path.append('/');
    path.append(_fileName);
    _output->appendOnboardIncludePath(path.view());
    _output->appendImplIncludePath("core/Try");
    _output->appendImplIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.view());
    _output->append(".gen.c\"\n");
}

void OnboardTypeSourceGen::appendEnumSerializer(const EnumType* type)
{
    _output->append("    switch(self) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case Photon");
        _output->append(_name);
        _output->append("_");
        _output->append(c->name());
        _output->append(":\n");
    }
    _output->append("        break;\n"
                    "    default:\n"
                    "        PHOTON_CRITICAL(\"Failed to serialize enum\");\n"
                    "        return PhotonError_InvalidValue;\n"
                    "    }\n    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self)");
    }, "Failed to write enum");
}

void OnboardTypeSourceGen::appendEnumDeserializer(const EnumType* type)
{
    _output->appendIndent();
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent();
    _output->append("Photon");
    _output->appendVarDecl(_name, "result");
    _output->appendIndent();
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &value)");
    }, "Failed to read enum");

    _output->append("    switch(value) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(c->value());
        _output->append(":\n        result = Photon");
        _output->append(_name);
        _output->append("_");
        _output->append(c->name());
        _output->append(";\n        break;\n");
    }
    _output->append("    default:\n"
                    "        PHOTON_WARNING(\"Failed to deserialize enum\");\n"
                    "        return PhotonError_InvalidValue;\n"
                    "    }\n"
                    "    *self = result;\n");
}

static bmcl::Option<std::size_t> typeFixedSize(const Type* type)
{
    return type->fixedSize();
}

void OnboardTypeSourceGen::appendStructSerializer(const StructType* type)
{
    InlineStructInspector inspector(_output, "self->");
    inspector.inspect<true, true>(type->fieldsRange(), &_inlineInspector);
}

void OnboardTypeSourceGen::appendStructDeserializer(const StructType* type)
{
    InlineStructInspector inspector(_output, "self->");
    inspector.inspect<true, false>(type->fieldsRange(), &_inlineInspector);
}

void OnboardTypeSourceGen::appendVariantSerializer(const VariantType* type)
{
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self->type)");
    }, "Failed to write variant type");

    _output->append("    switch(self->type) {\n");
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case Photon");
        _output->append(_name);
        _output->append("Type_");
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
                argName.append(_name);
                argName.append("._");
                argName.appendNumericValue(j);
                _inlineInspector.inspect<true, true>(t, ctx, argName.view());
                argName.resize(11);
                j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(_name);
                argName.append(".");
                argName.append(f->name());
                _inlineInspector.inspect<true, true>(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n"
                        "    }\n");
    }
    _output->append("    default:\n"
                    "        PHOTON_CRITICAL(\"Failed to serialize variant\");\n"
                    "        return PhotonError_InvalidValue;\n"
                    "    }\n");
}

void OnboardTypeSourceGen::appendVariantDeserializer(const VariantType* type)
{
    _output->appendIndent();
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent();
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &value)");
    }, "Failed to read variant type");

    _output->append("    switch(value) {\n");
    std::size_t i = 0;
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(i);
        i++;
        _output->append(": {\n        self->type = Photon");
        _output->append(_name);
        _output->append("Type_");
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
                    argName.append(_name);
                    argName.append("._");
                    argName.appendNumericValue(j);
                    _inlineInspector.inspect<true, false>(t, ctx, argName.view());
                    argName.resize(11);
                    j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(_name);
                argName.append(".");
                argName.append(f->name());
                _inlineInspector.inspect<true, false>(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n    }\n");
        _output->append("");
    }
    _output->append("    default:\n"
                    "        PHOTON_WARNING(\"Failed to deserialize variant\");\n"
                    "        return PhotonError_InvalidValue;\n"
                    "    }\n");
}

void OnboardTypeSourceGen::appendDynArraySerializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->append("    if (self->size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_CRITICAL(\"Failed to serialize dynarray\");\n"
                    "        return PhotonError_InvalidValue;\n    }\n"
                    "    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, self->size)");
    }, "Failed to write dynarray size");
    auto size = typeFixedSize(type->elementType());
    if (size.isSome()) {
        _inlineInspector.appendSizeCheck<true, true>(ctx, "self->size * " + std::to_string(size.unwrap()), _output);
    }
    _output->appendLoopHeader(ctx, "self->size");
    InlineSerContext lctx = ctx.indent();
    _inlineInspector.inspect<true, true>(type->elementType(), lctx, "self->data[a]", size.isNone());
    _output->append("    }\n");
}

void OnboardTypeSourceGen::appendDynArrayDeserializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->appendIndent();
    _output->appendVarDecl("uint64_t", "size");
    _output->appendIndent();
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVaruint(src, &size)");
    }, "Failed to read dynarray size");
    _output->append("    if (size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_WARNING(\"Failed to deserialize dynarray\");\n"
                    "        return PhotonError_InvalidValue;\n    }\n");
    auto size = typeFixedSize(type->elementType());
    if (size.isSome()) {
        _inlineInspector.appendSizeCheck<true, false>(ctx, "size * " + std::to_string(size.unwrap()), _output);
    }
    _output->appendLoopHeader(ctx, "size");
    InlineSerContext lctx = ctx.indent();
    _inlineInspector.inspect<true, false>(type->elementType(), lctx, "self->data[a]", size.isNone());
    _output->append("    }\n");
    if (type->elementType()->isBuiltinChar()) {
        _output->append("    self->data[size] = '\\0';\n");
    }
    _output->append("    self->size = size;\n");
}

template <typename T, typename F>
void OnboardTypeSourceGen::genSource(const T* type, F&& serGen, F&& deserGen)
{
    _output->appendEol();
    _prototypeGen.appendTypeSerializerFunctionPrototype(_baseType);
    _output->append("\n{\n");
    (this->*serGen)(type);
    _output->append("    return PhotonError_Ok;\n}\n");
    _output->appendEol();
    _prototypeGen.appendTypeDeserializerFunctionPrototype(_baseType);
    _output->append("\n{\n");
    (this->*deserGen)(type);
    _output->append("    return PhotonError_Ok;\n"
                    "}\n\n"
                    "#undef _PHOTON_FNAME");
    _output->appendEol();
}

bool OnboardTypeSourceGen::visitDynArrayType(const DynArrayType* type)
{
    _output->appendPragmaOnce(); //HACK
    SrcBuilder path("_dynarray_/");
    TypeNameGen gen(&path);
    gen.genTypeName(type);
    _output->appendOnboardIncludePath(path.view());
    _output->appendImplIncludePath("core/Try");
    _output->appendImplIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.view());
    _output->append(".gen.c\"\n\n");
    _prototypeGen.appendTypeSerializerFunctionPrototype(type);
    _output->append("\n{\n");
    appendDynArraySerializer(type);
    _output->append("    return PhotonError_Ok;\n}\n");
    _output->appendEol();
    _prototypeGen.appendTypeDeserializerFunctionPrototype(type);
    _output->append("\n{\n");
    appendDynArrayDeserializer(type);
    _output->append("    return PhotonError_Ok;\n"
                    "}\n\n"
                    "#undef _PHOTON_FNAME");
    _output->appendEol();
    return false;
}

bool OnboardTypeSourceGen::visitEnumType(const EnumType* type)
{
    genSource(type, &OnboardTypeSourceGen::appendEnumSerializer, &OnboardTypeSourceGen::appendEnumDeserializer);
    return false;
}

bool OnboardTypeSourceGen::visitStructType(const StructType* type)
{
    genSource(type, &OnboardTypeSourceGen::appendStructSerializer, &OnboardTypeSourceGen::appendStructDeserializer);
    return false;
}

bool OnboardTypeSourceGen::visitVariantType(const VariantType* type)
{
    genSource(type, &OnboardTypeSourceGen::appendVariantSerializer, &OnboardTypeSourceGen::appendVariantDeserializer);
    return false;
}

void OnboardTypeSourceGen::genTypeSource(const NamedType* type, bmcl::StringView name)
{
    _name = name;
    _fileName = type->name();
    _baseType = type;
    genSource(type, type->moduleName());
}

void OnboardTypeSourceGen::genTypeSource(const GenericInstantiationType* instantiation, bmcl::StringView name)
{
    _name = name;
    _fileName = name;
    _baseType = instantiation;
    const Type* type = instantiation->instantiatedType()->resolveFinalType();
    _output->appendPragmaOnce(); //HACK
    genSource(type, "_generic_");

}

void OnboardTypeSourceGen::genSource(const Type* type, bmcl::StringView modName)
{
    _output->appendPragmaOnce(); //HACK
    switch (type->typeKind()) {
        case TypeKind::Variant:
            appendIncludes(modName);
            visitVariantType(type->asVariant());
            return;
        case TypeKind::Struct:
            appendIncludes(modName);
            visitStructType(type->asStruct());
            return;
        case TypeKind::Enum:
            appendIncludes(modName);
            visitEnumType(type->asEnum());
            return;
        default:
            return;
    }
}

void OnboardTypeSourceGen::genTypeSource(const DynArrayType* type)
{
    _baseType = type;
    visitDynArrayType(type);
}
}
