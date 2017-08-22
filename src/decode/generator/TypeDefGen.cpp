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

TypeDefGen::TypeDefGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
{
}

TypeDefGen::~TypeDefGen()
{
}

inline bool TypeDefGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}

void TypeDefGen::genTypeDef(const Type* type)
{
    traverseType(type);
}

void TypeDefGen::genComponentDef(const Component* comp)
{
    if (!comp->hasParams()) {
        return;
    }
    appendFieldVec(comp->paramsRange(), bmcl::StringView::empty());
}

bool TypeDefGen::visitEnumType(const EnumType* type)
{
    appendEnum(type);
    return false;
}

bool TypeDefGen::visitStructType(const StructType* type)
{
    appendStruct(type);
    return false;
}

bool TypeDefGen::visitVariantType(const VariantType* type)
{
    appendVariant(type);
    return false;
}

bool TypeDefGen::visitDynArrayType(const DynArrayType* type)
{
    TypeNameGen gen(_output);
    _output->appendTagHeader("struct");

    _output->appendIndent(1);
    _typeReprGen->genTypeRepr(type->elementType());
    _output->append(" data[");
    _output->appendNumericValue(type->maxSize());
    _output->append("];\n");

    _output->appendIndent(1);
    _output->append("size_t size;\n");

    _output->append("} Photon");
    gen.genTypeName(type);
    _output->append(";\n");
    _output->appendEol();
    return false;
}

void TypeDefGen::appendFieldVec(TypeVec::ConstRange fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    std::size_t i = 1;
    for (const Type* type : fields) {
        _output->appendIndent(1);
        _typeReprGen->genTypeRepr(type, "_" + std::to_string(i));
        _output->append(";\n");
        i++;
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendFieldVec(FieldVec::ConstRange fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    for (const Field* field : fields) {
        _output->appendIndent(1);
        _typeReprGen->genTypeRepr(field->type(), field->name());
        _output->append(";\n");
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendStruct(const StructType* type)
{
    appendFieldVec(type->fieldsRange(), type->name());
}

void TypeDefGen::appendEnum(const EnumType* type)
{
    _output->appendTagHeader("enum");

    for (const EnumConstant* c : type->constantsRange()) {
        _output->appendIndent(1);
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(c->name().toStdString());
        if (c->isUserSet()) {
            _output->append(" = ");
            _output->append(std::to_string(c->value()));
        }
        _output->append(",\n");
    }

    _output->appendTagFooter(type->name());
    _output->appendEol();
}

void TypeDefGen::appendVariant(const VariantType* type)
{
    std::vector<bmcl::StringView> fieldNames;

    _output->appendTagHeader("enum");

    for (const VariantField* field : type->fieldsRange()) {
        _output->appendIndent(1);
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type_");
        _output->append(field->name());
        _output->append(",\n");
    }

    _output->append("} ");
    _output->appendModPrefix();
    _output->append(type->name());
    _output->append("Type;\n");
    _output->appendEol();

    for (const VariantField* field : type->fieldsRange()) {
        switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                auto types = static_cast<const TupleVariantField*>(field)->typesRange();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                appendFieldVec(types, name);
                break;
            }
            case VariantFieldKind::Struct: {
                auto fields = static_cast<const StructVariantField*>(field)->fieldsRange();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                appendFieldVec(fields, name);
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
        _output->appendModPrefix();
        _output->append(field->name());
        _output->append(type->name());
        _output->appendSpace();
        _output->appendWithFirstLower(field->name());
        _output->append(type->name());
        _output->append(";\n");
    }

    _output->append("    } data;\n");
    _output->appendIndent(1);
    _output->appendModPrefix();
    _output->append(type->name());
    _output->append("Type");
    _output->append(" type;\n");

    _output->appendTagFooter(type->name());
    _output->appendEol();
}

bool TypeDefGen::visitAliasType(const AliasType* type)
{
    _output->append("typedef ");
    bmcl::StringView modName;
    if (type->moduleName() != "core") {
        modName = type->moduleName();
    }
    const Type* link = type->alias();
    if (link->isFunction()) {
        StringBuilder typedefName("Photon");
        typedefName.appendWithFirstUpper(modName);
        typedefName.append(type->name());
        _typeReprGen->genTypeRepr(link, typedefName.result());
    } else {
        _typeReprGen->genTypeRepr(link);
        _output->append(" Photon");
        _output->appendWithFirstUpper(modName);
        _output->append(type->name());
    }
    _output->append(";\n\n");
    return false;
}
}
