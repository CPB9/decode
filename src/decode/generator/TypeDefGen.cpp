/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeDefGen.h"
#include "decode/ast/Component.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"

namespace decode {

TypeDefGen::TypeDefGen(SrcBuilder* output)
    : _output(output)
{
}

TypeDefGen::~TypeDefGen()
{
}

void TypeDefGen::genTypeDef(const TopLevelType* type, bmcl::StringView name)
{
    switch (type->typeKind()) {
    case TypeKind::Struct:
        appendStruct(type->asStruct(), name);
        break;
    case TypeKind::Enum:
        appendEnum(type->asEnum(), name);
        break;
    case TypeKind::Variant:
        appendVariant(type->asVariant(), name);
        break;
    case TypeKind::Alias:
        appendAlias(type->asAlias(), name);
        break;
    case TypeKind::GenericInstantiation:
        genTypeDef(type->asGenericInstantiation()->instantiatedType(), name);
        break;
    default:
        return;
    }
}

void TypeDefGen::genTypeDef(const DynArrayType* type)
{
    appendDynArray(type);
}

void TypeDefGen::genComponentDef(const Component* comp)
{
    if (!comp->hasVars()) {
        return;
    }
    appendFieldVec(comp->varsRange(), comp->name());
}

void TypeDefGen::appendDynArray(const DynArrayType* type)
{
    TypeNameGen gen(_output);
    _output->appendTagHeader("struct");

    TypeReprGen reprGen(_output);
    _output->appendIndent();
    reprGen.genOnboardTypeRepr(type->elementType());
    _output->append(" data[");
    _output->appendNumericValue(type->maxSize());
    if (type->elementType()->isBuiltinChar()) {
        _output->append(" + 1");
    }
    _output->append("];\n");

    _output->appendIndent(1);
    _output->append("uint64_t size;\n");

    _output->append("} Photon");
    gen.genTypeName(type);
    _output->append(";\n");
    _output->appendEol();
}

void TypeDefGen::appendFieldVec(TypeVec::ConstRange fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    TypeReprGen reprGen(_output);
    std::size_t i = 1;
    for (const Type* type : fields) {
        _output->appendIndent();
        reprGen.genOnboardTypeRepr(type, "_" + std::to_string(i));
        _output->append(";\n");
        i++;
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendFieldVec(FieldVec::ConstRange fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    TypeReprGen reprGen(_output);
    for (const Field* field : fields) {
        _output->appendIndent();
        reprGen.genOnboardTypeRepr(field->type(), field->name());
        _output->append(";\n");
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendStruct(const StructType* type, bmcl::StringView name)
{
    appendFieldVec(type->fieldsRange(), name);
}

void TypeDefGen::appendEnum(const EnumType* type, bmcl::StringView name)
{
    _output->appendTagHeader("enum");

    for (const EnumConstant* c : type->constantsRange()) {
        _output->appendIndent(1);
        _output->append("Photon");
        _output->append(name);
        _output->append("_");
        _output->append(c->name().toStdString());
        if (c->isUserSet()) {
            _output->append(" = ");
            _output->append(std::to_string(c->value()));
        }
        _output->append(",\n");
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendVariant(const VariantType* type, bmcl::StringView name)
{
    std::vector<bmcl::StringView> fieldNames;

    _output->appendTagHeader("enum");

    for (const VariantField* field : type->fieldsRange()) {
        _output->appendIndent(1);
        _output->append("Photon");
        _output->append(name);
        _output->append("Type_");
        _output->append(field->name());
        _output->append(",\n");
    }

    _output->append("} ");
    _output->append("Photon");
    _output->append(name);
    _output->append("Type;\n");
    _output->appendEol();

    for (const VariantField* field : type->fieldsRange()) {
        switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                auto types = static_cast<const TupleVariantField*>(field)->typesRange();
                std::string fieldName = field->name().toStdString();
                fieldName.append(name.begin(), name.end());
                appendFieldVec(types, fieldName);
                break;
            }
            case VariantFieldKind::Struct: {
                auto fields = static_cast<const StructVariantField*>(field)->fieldsRange();
                std::string fieldName = field->name().toStdString();
                fieldName.append(name.begin(), name.end());
                appendFieldVec(fields, fieldName);
                break;
            }
        }
    }

    _output->appendTagHeader("struct");
    _output->append("    union {\n");

    for (const VariantField* field : type->fieldsRange()) {
        if (field->variantFieldKind() == VariantFieldKind::Constant) {
            continue;
        }
        _output->append("        ");
        _output->append("Photon");
        _output->append(field->name());
        _output->append(name);
        _output->appendSpace();
        _output->appendWithFirstLower(field->name());
        _output->append(name);
        _output->append(";\n");
    }

    _output->append("    } data;\n");
    _output->appendIndent(1);
    _output->append("Photon");
    _output->append(name);
    _output->append("Type");
    _output->append(" type;\n");

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendAlias(const AliasType* type, bmcl::StringView name)
{
    _output->append("typedef ");
    const Type* link = type->alias();
    TypeReprGen reprGen(_output);
    if (link->isFunction()) {
        StringBuilder typedefName("Photon");
        typedefName.appendWithFirstUpper(name);
        reprGen.genOnboardTypeRepr(link, typedefName.view());
    } else {
        reprGen.genOnboardTypeRepr(link);
        _output->append(" Photon");
        _output->appendWithFirstUpper(name);
    }
    _output->append(";\n\n");
}
}
