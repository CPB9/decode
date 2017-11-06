/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/GcTypeGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/core/Foreach.h"
#include "decode/ast/Type.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/IncludeGen.h"

#include <bmcl/StringView.h>

namespace decode {

GcTypeGen::GcTypeGen(SrcBuilder* output)
    : _output(output)
{
}

GcTypeGen::~GcTypeGen()
{
}

void GcTypeGen::appendFullTypeName(const NamedType* type)
{
    _output->append(type->moduleName());
    _output->append("::");
    _output->append(type->name());
}

void GcTypeGen::appendEnumConstantName(const EnumType* type, const EnumConstant* constant)
{
    _output->append(type->name());
    _output->append("::");
    _output->append(constant->name());
}

void GcTypeGen::generateHeader(const TopLevelType* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::DynArray:
    case TypeKind::Function:
    case TypeKind::Imported:
    case TypeKind::Alias:
    case TypeKind::GenericInstantiation:
    case TypeKind::GenericParameter:
        return;
    case TypeKind::Enum:
        generateEnum(type->asEnum());
        break;
    case TypeKind::Struct:
        generateStruct(type->asStruct());
        break;
    case TypeKind::Variant:
    case TypeKind::Generic:
        break;
    }
}

void GcTypeGen::beginNamespace(bmcl::StringView modName)
{
    _output->append("namespace photongen {\nnamespace ");
    _output->append(modName);
    _output->append(" {\n\n");
}

void GcTypeGen::endNamespace()
{
    _output->append("}\n}\n");
}

void GcTypeGen::appendSerPrefix(const NamedType* type)
{
    _output->append("inline bool serialize");
    _output->appendWithFirstUpper(type->name());
    _output->append("(const ");
    _output->appendWithFirstUpper(type->name());
    _output->append("& self, bmcl::MemWriter* dest, photon::CoderState* state)\n{\n");
}

void GcTypeGen::appendDeserPrefix(const NamedType* type)
{
    _output->append("inline bool deserialize");
    _output->appendWithFirstUpper(type->name());
    _output->append("(");
    _output->appendWithFirstUpper(type->name());
    _output->append("* self, bmcl::MemReader* src, photon::CoderState* state)\n{\n");
}

void GcTypeGen::generateEnum(const EnumType* type)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("bmcl/MemWriter.h");
    _output->appendInclude("photon/model/CoderState.h");
    _output->appendEol();

    beginNamespace(type->moduleName());

    //type
    _output->append("enum class ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\n");

    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    ");
        _output->append(c->name());
        _output->append(" = ");
        _output->appendNumericValue(c->value());
        _output->append(",\n");
    }

    _output->append("}\n\n");

    //ser
    appendSerPrefix(type);

    _output->append("    switch(self) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        appendEnumConstantName(type, c);
        _output->append(":\n");
    }
    _output->append("        break;\n"
                    "    default:\n"
                    "        state->setError(\"Could not serialize enum `");
    appendFullTypeName(type);
    _output->append("` with invalid value (\" + std::to_string(self) + \")\");\n"
                    "        return false;\n"
                    "    }\n    "
                    "if(!dest->writeVarint((int64_t)self)) {\n"
                    "        state->setError(\"Not enough space to serialize enum `");
    appendFullTypeName(type);
    _output->append("`\");\n        return false;\n    }\n"
                    "    return true;\n}\n\n");

    //deser
    appendDeserPrefix(type);
    _output->append("    int64_t value;\n    if (!src->readVarint(&value)) {\n"
                    "        state->setError(\"Not enough data to deserialize enum `");
    appendFullTypeName(type);
    _output->append("`\");\n        return false;\n    }\n");

    _output->append("    switch(value) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(c->value());
        _output->append(":\n        *self = ");
        appendEnumConstantName(type, c);
        _output->append(";\n        return true;\n");
    }
    _output->append("    }\n    state->setError(\"Failed to deserialize enum `");
    appendFullTypeName(type);
    _output->append("`, got invalid value (\" + std::to_string(value) + \")\");\n"
                    "    return false;\n");

    endNamespace();
}

void GcTypeGen::generateStruct(const StructType* type)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("cstddef");
    _output->appendInclude("cstdint");
    _output->appendInclude("cstdbool");
    _output->appendEol();

    TypeDependsCollector coll;
    TypeDependsCollector::Depends depends;
    coll.collect(type, &depends);
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(&depends);
    if (!depends.empty()) {
        _output->appendEol();
    }

    beginNamespace(type->moduleName());

    _output->append("struct ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\npublic:\n");

    _output->appendIndent();
    _output->appendWithFirstUpper(type->name());
    _output->append("()\n");

    auto constructVars = [this](const StructType* type, bool isNull) {
        char c = ':';
        for (const Field* field : type->fieldsRange()) {
            const Type* t = field->type()->resolveFinalType();
            if (!t->isBuiltin() && !t->isReference()) {
                continue;
            }

            _output->append("        ");
            _output->append(c);
            _output->append(" _");
            _output->append(field->name());
            _output->append('(');
            if (isNull) {
                if (t->isBuiltin()) {
                    _output->append('0');
                } else if (t->isReference()) {
                    _output->append("nullptr");
                }
            } else {
                _output->append(field->name());
            }
            _output->append(")\n");
            c = ',';
        }
    };
    constructVars(type, true);
    _output->append("    {\n    }\n\n");

    Rc<ReferenceType> ref = new ReferenceType(ReferenceKind::Reference, false, nullptr);
    auto wrapType = [&](const Type* t, bool isMutable) -> const Type* {
        switch (t->typeKind()) {
        case TypeKind::Builtin:
        case TypeKind::Reference:
        case TypeKind::Array:
        case TypeKind::Function:
        case TypeKind::GenericParameter:
            return t;
        case TypeKind::DynArray:
        case TypeKind::Enum:
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Imported:
        case TypeKind::Alias:
        case TypeKind::Generic:
        case TypeKind::GenericInstantiation:
            ref->setMutable(isMutable);
            ref->setPointee(const_cast<Type*>(t));
            return ref.get();
        }
    };

    TypeReprGen gen(_output);
    _output->appendIndent();
    _output->appendWithFirstUpper(type->name());
    _output->append('(');
    foreachList(type->fieldsRange(), [&](const Field* arg) {
        const Type* t = wrapType(arg->type()->resolveFinalType(), false);
        gen.genGcTypeRepr(t, arg->name());
    }, [this](const Field*) {
         _output->append(", ");
    });

    _output->append(")\n");
    constructVars(type, false);
    _output->append("    {\n    }\n\n");

    for (const Field* field : type->fieldsRange()) {
        _output->append("    void set");
        _output->appendWithFirstUpper(field->name());
        _output->append('(');

        Rc<const Type> t = wrapType(field->type()->resolveFinalType(), false);
        gen.genGcTypeRepr(t.get(), field->name());
        _output->append(")\n    {\n        _");
        _output->append(field->name());
        _output->append(" = ");
        _output->append(field->name());
        _output->append(";\n    }\n\n");
    }

    auto appendGetter = [&](const Field* field, bool isMutable) {
        _output->append("    ");
        Rc<const Type> t = field->type()->resolveFinalType();
        if (isMutable) {
            ref->setPointee(const_cast<Type*>(t.get()));
            ref->setMutable(isMutable);
            t = ref;
        } else {
            t = wrapType(field->type()->resolveFinalType(), false);
        }
        gen.genGcTypeRepr(t.get());
        _output->append(' ');
        _output->append(field->name());
        _output->append("()");
        if (!isMutable) {
            _output->append(" const");
        }
        _output->append("\n    {\n        return _");
        _output->append(field->name());
        _output->append(";\n    }\n\n");
    };

    for (const Field* field : type->fieldsRange()) {
        appendGetter(field, false);
        appendGetter(field, true);
    }

    _output->append("private:\n");
    SrcBuilder builder("_");
    for (const Field* field : type->fieldsRange()) {
        _output->appendIndent();
        builder.append(field->name());
        gen.genGcTypeRepr(field->type(), builder.view());
        builder.resize(1);
        _output->append(";\n");
    }

    _output->append("}\n\n");

    //ser
    appendSerPrefix(type);

    _output->append("}\n");

    endNamespace();
}
}
