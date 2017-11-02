/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeReprGen.h"
#include "decode/core/Foreach.h"
#include "decode/generator/TypeNameGen.h"

#include <bmcl/Logging.h>

namespace decode {

TypeReprGen::TypeReprGen(SrcBuilder* dest)
    : _output(dest)
    , _currentOffset(0)
{
}

TypeReprGen::~TypeReprGen()
{
}

static bmcl::StringView builtinToC(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
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
    case BuiltinTypeKind::F32:
        return "float";
    case BuiltinTypeKind::F64:
        return "double";
    case BuiltinTypeKind::Bool:
        return "bool";
    case BuiltinTypeKind::Void:
        return "void";
    case BuiltinTypeKind::Char:
        return "char";
    }
    return nullptr;
}

void TypeReprGen::writeBuiltin(const BuiltinType* type)
{
    _output->insert(_currentOffset, builtinToC(type));
}

void TypeReprGen::writeArray(const ArrayType* type)
{
    _output->append('[');
    _output->append(std::to_string(type->elementCount()));
    _output->append(']');
    writeType(type->elementType());
}

void TypeReprGen::writePointer(const ReferenceType* type)
{
    if (type->isMutable()) {
        _output->insert(_currentOffset,"* ");
    } else {
        _output->insert(_currentOffset, " const* ");
    }
    writeType(type->pointee());
}

void TypeReprGen::writeNamed(const Type* type)
{
    _temp.append("Photon");
    TypeNameGen sng(&_temp);
    sng.genTypeName(type);
    _output->insert(_currentOffset, _temp.result());
    _temp.clear();
}

void TypeReprGen::writeFunction(const FunctionType* type)
{
    _output->insert(_currentOffset, "(*");
    _output->append(")(");
    std::size_t oldOffset = _currentOffset;
    _currentOffset = _output->result().size();
    foreachList(type->argumentsRange(), [this](const Field* arg) {
        writeType(arg->type());
    }, [this](const Field*) {
         _output->append(", ");
         _currentOffset = _output->result().size();
    });
    _currentOffset = oldOffset;
    _output->append(')');
    if (type->returnValue().isNone()) {
        _output->insert(_currentOffset, "void ");
    } else {
        writeType(type->returnValue().unwrap());
    }
}

void TypeReprGen::writeType(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        writeBuiltin(type->asBuiltin());
        break;
    case TypeKind::Reference:
        writePointer(type->asReference());
        break;
    case TypeKind::Array:
        writeArray(type->asArray());
        break;
    case TypeKind::DynArray:
        writeNamed(type);
        break;
    case TypeKind::Function:
        writeFunction(type->asFunction());
        break;
    case TypeKind::Enum:
        writeNamed(type);
        break;
    case TypeKind::Struct:
        writeNamed(type);
        break;
    case TypeKind::Variant:
        writeNamed(type);
        break;
    case TypeKind::Imported:
        writeType(type->asImported()->link());
        break;
    case TypeKind::Alias:
        writeType(type->asAlias()->alias());
        break;
    case TypeKind::Generic:
        //TODO:
        break;
    case TypeKind::GenericInstantiation:
        writeNamed(type);
        break;
    case TypeKind::GenericParameter:
        _output->insert(_currentOffset, type->asGenericParemeter()->name());
        break;
    }
}

void TypeReprGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _currentOffset = _output->result().size();
    if (!fieldName.isEmpty()) {
        _output->append(' ');
    }
    _output->append(fieldName);
    writeType(type);
}

}
