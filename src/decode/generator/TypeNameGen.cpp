/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeNameGen.h"
#include "decode/generator/StringBuilder.h"
#include "decode/ast/Decl.h"

#include <bmcl/StringView.h>

namespace decode {

TypeNameGen::TypeNameGen(SrcBuilder* dest)
    : _output(dest)
{
}

TypeNameGen::~TypeNameGen()
{
}

bool TypeNameGen::visitBuiltinType(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        _output->append("USize");
        break;
    case BuiltinTypeKind::ISize:
        _output->append( "ISize");
        break;
    case BuiltinTypeKind::Varuint:
        _output->append( "Varuint");
        break;
    case BuiltinTypeKind::Varint:
        _output->append( "Varint");
        break;
    case BuiltinTypeKind::U8:
        _output->append( "U8");
        break;
    case BuiltinTypeKind::I8:
        _output->append( "I8");
        break;
    case BuiltinTypeKind::U16:
        _output->append( "U16");
        break;
    case BuiltinTypeKind::I16:
        _output->append( "I16");
        break;
    case BuiltinTypeKind::U32:
        _output->append( "U32");
        break;
    case BuiltinTypeKind::I32:
        _output->append( "I32");
        break;
    case BuiltinTypeKind::U64:
        _output->append( "U64");
        break;
    case BuiltinTypeKind::I64:
        _output->append( "I64");
        break;
    case BuiltinTypeKind::F32:
        _output->append( "F32");
        break;
    case BuiltinTypeKind::F64:
        _output->append( "F64");
        break;
    case BuiltinTypeKind::Bool:
        _output->append( "Bool");
        break;
    case BuiltinTypeKind::Void:
        _output->append( "Void");
        break;
    case BuiltinTypeKind::Char:
        _output->append( "Char");
        break;
    }
    return false;
}

bool TypeNameGen::visitArrayType(const ArrayType* type)
{
    _output->append("ArrOf");
    return true;
}

bool TypeNameGen::visitReferenceType(const ReferenceType* type)
{
    if (type->isMutable()) {
        _output->append("Mut");
    }
    switch (type->referenceKind()) {
    case ReferenceKind::Pointer:
        _output->append("PtrTo");
        break;
    case ReferenceKind::Reference:
        _output->append("RefTo");
        break;
    }
    return true;
}

bool TypeNameGen::visitDynArrayType(const DynArrayType* type)
{
    _output->append("DynArrayOf");
    traverseType(type->elementType());
    _output->append("MaxSize");
    _output->appendNumericValue(type->maxSize());
    return false;
}

bool TypeNameGen::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    if (type->moduleName() != "core") {
        _output->appendWithFirstUpper(type->moduleName());
    }
    _output->append(type->genericName().toStdString());
    for (const Type* t : type->substitutedTypesRange()) {
        ascendTypeOnce(t);
    }
    return false;
}

bool TypeNameGen::appendTypeName(const NamedType* type)
{
    if (type->moduleName() != "core") {
        _output->appendWithFirstUpper(type->moduleName());
    }
    _output->append(type->name());
    return false;
}

void TypeNameGen::genTypeName(const Type* type)
{
    traverseType(type);
}

std::string TypeNameGen::genTypeNameAsString(const Type* type)
{
    SrcBuilder output;
    TypeNameGen gen(&output);
    gen.genTypeName(type);
    return std::move(output.result());
}
}
