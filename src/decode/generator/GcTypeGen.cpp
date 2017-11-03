/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/GcTypeGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/ast/Type.h"
#include "decode/generator/GcInlineTypeSerializerGen.h"

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
    _output->append("inline bool serialize");
    _output->appendWithFirstUpper(type->name());
    _output->append("(");
    _output->appendWithFirstUpper(type->name());
    _output->append(" self, bmcl::MemWriter* dest, photon::CoderState* state)\n{\n");

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
    _output->append("inline bool deserialize");
    _output->appendWithFirstUpper(type->name());
    _output->append("(");
    _output->appendWithFirstUpper(type->name());
    _output->append("* self, bmcl::MemReader* src, photon::CoderState* state)\n{\n"
                    "    int64_t value;\n    if (!src->readVarint(&value)) {\n"
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

    beginNamespace(type->moduleName());

    _output->append("struct ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\npublic:\n");


    _output->append("private:\n");




    _output->append("}\n");
    endNamespace();
}
}
