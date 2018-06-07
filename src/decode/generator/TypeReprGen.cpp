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
#include "decode/ast/Type.h"

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

template <bool isOnboard>
void TypeReprGen::writeBuiltin(const BuiltinType* type)
{
    auto insertStd = [this]() {
        if (!isOnboard) {
            _output->insert(_currentOffset, "std::");
        }
    };
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        _output->insert(_currentOffset, "size_t");
        insertStd();
        break;
    case BuiltinTypeKind::ISize:
        _output->insert(_currentOffset, "ptrdiff_t");
        insertStd();
        break;
    case BuiltinTypeKind::Varuint:
        _output->insert(_currentOffset, "uint64_t");
        insertStd();
        break;
    case BuiltinTypeKind::Varint:
        _output->insert(_currentOffset, "int64_t");
        insertStd();
        break;
    case BuiltinTypeKind::U8:
        _output->insert(_currentOffset, "uint8_t");
        insertStd();
        break;
    case BuiltinTypeKind::I8:
        _output->insert(_currentOffset, "int8_t");
        insertStd();
        break;
    case BuiltinTypeKind::U16:
        _output->insert(_currentOffset, "uint16_t");
        insertStd();
        break;
    case BuiltinTypeKind::I16:
        _output->insert(_currentOffset, "int16_t");
        insertStd();
        break;
    case BuiltinTypeKind::U32:
        _output->insert(_currentOffset, "uint32_t");
        insertStd();
        break;
    case BuiltinTypeKind::I32:
        _output->insert(_currentOffset, "int32_t");
        insertStd();
        break;
    case BuiltinTypeKind::U64:
        _output->insert(_currentOffset, "uint64_t");
        insertStd();
        break;
    case BuiltinTypeKind::I64:
        _output->insert(_currentOffset, "int64_t");
        insertStd();
        break;
    case BuiltinTypeKind::F32:
        _output->insert(_currentOffset, "float");
        break;
    case BuiltinTypeKind::F64:
        _output->insert(_currentOffset, "double");
        break;
    case BuiltinTypeKind::Bool:
        _output->insert(_currentOffset, "bool");
        break;
    case BuiltinTypeKind::Void:
        _output->insert(_currentOffset, "void");
        break;
    case BuiltinTypeKind::Char:
        _output->insert(_currentOffset, "char");
        break;
    }
}

template <bool isOnboard>
void TypeReprGen::writeArray(const ArrayType* type)
{
    if (isOnboard) {
        _output->append('[');
        _output->append(std::to_string(type->elementCount()));
        _output->append(']');
        writeType<isOnboard>(type->elementType());
    } else {
        _temp.append("std::array<");
        TypeReprGen gen(&_temp);
        gen.genTypeRepr<false>(type->elementType());
        _temp.append(", ");
        _temp.appendNumericValue(type->elementCount());
        _temp.append(">");
        _output->insert(_currentOffset, _temp.view());
        _temp.clear();
    }
}

template <bool isOnboard>
void TypeReprGen::writePointer(const ReferenceType* type)
{
    char ptrPrefix;
    bmcl::StringView constPrefix;
    if (type->referenceKind() == ReferenceKind::Pointer) {
        ptrPrefix = '*';
    } else {
        if (isOnboard) {
            ptrPrefix = '*';
        } else {
            ptrPrefix = '&';
        }
    }
    if (!type->isMutable()) {
        constPrefix = "const";
    }

    switch (type->pointee()->resolveFinalType()->typeKind()) {
    case TypeKind::Enum:
    case TypeKind::Struct:
    case TypeKind::GenericInstantiation:
    case TypeKind::DynArray:
    case TypeKind::Variant:
    case TypeKind::Builtin:
        _output->insert(_currentOffset, ptrPrefix);
        writeType<isOnboard>(type->pointee());
        _output->insert(_currentOffset, ' ');
        _output->insert(_currentOffset, constPrefix);
        break;
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Generic:
    case TypeKind::GenericParameter:
    case TypeKind::Imported:
    case TypeKind::Alias:
        _output->insert(_currentOffset, ptrPrefix);
        _output->insert(_currentOffset, constPrefix);
        _output->insert(_currentOffset, ' ');
        writeType<isOnboard>(type->pointee());
        break;
    }
}

void TypeReprGen::writeOnboardTypeName(const Type* type)
{
    _temp.append("Photon");
    TypeNameGen sng(&_temp);
    sng.genTypeName(type);
    _output->insert(_currentOffset, _temp.view());
    _temp.clear();
}

template <bool isOnboard>
void TypeReprGen::writeNamed(const NamedType* type)
{
    writeNamed<isOnboard>(type, type);
}

template <bool isOnboard>
void TypeReprGen::writeNamed(const NamedType* type, const NamedType* origin, bool originIsGeneric)
{
    if (isOnboard) {
        _temp.append("Photon");
        if (origin->moduleName() != "core") {
            _temp.appendWithFirstUpper(origin->moduleName());
        }
        _temp.appendWithFirstUpper(type->name());
    } else {
        _temp.append("photongen::");
        _temp.append(origin->moduleName());
        _temp.append("::");
        _temp.appendWithFirstUpper(type->name());

        if (originIsGeneric) {
            _temp.append("<");
            foreachList(origin->asGeneric()->parametersRange(), [&](const GenericParameterType* t) {
                _temp.append(t->name());
            }, [this](const Type*) {
                _temp.append(", ");
            });
            _temp.append(">");
        }
    }
    _output->insert(_currentOffset, _temp.view());
    _temp.clear();
}

template <bool isOnboard>
void TypeReprGen::writeFunction(const FunctionType* type)
{
    _output->insert(_currentOffset, "(*");
    _output->append(")(");
    std::size_t oldOffset = _currentOffset;
    _currentOffset = _output->size();
    foreachList(type->argumentsRange(), [this](const Field* arg) {
        writeType<isOnboard>(arg->type());
    }, [this](const Field*) {
         _output->append(", ");
         _currentOffset = _output->size();
    });
    _currentOffset = oldOffset;
    _output->append(')');
    if (type->returnValue().isNone()) {
        _output->insert(_currentOffset, "void ");
    } else {
        writeType<isOnboard>(type->returnValue().unwrap());
    }
}

template <bool isOnboard>
void TypeReprGen::writeGenericInstantiation(const GenericInstantiationType* type)
{
    if (isOnboard) {
        writeOnboardTypeName(type);
    } else {
        _temp.append("photongen::");
        _temp.append(type->moduleName());
        _temp.append("::");
        _temp.append(type->genericName());
        _temp.append("<");

        TypeReprGen gen(&_temp);
        foreachList(type->substitutedTypesRange(), [&](const Type* t) {
            gen.genTypeRepr<false>(t);
        }, [this](const Type*) {
            _temp.append(", ");
        });
        _temp.append(">");
        _output->insert(_currentOffset, _temp.view());
        _temp.clear();
    }
}

template <bool isOnboard>
void TypeReprGen::writeDynArray(const DynArrayType* type)
{
    if (isOnboard) {
        writeOnboardTypeName(type);
    } else {
        if (type->elementType()->isBuiltinChar()) {
            _output->insert(_currentOffset, "std::string");
            return;
        }
        _temp.append("std::vector<");
        TypeReprGen gen(&_temp);
        gen.genTypeRepr<false>(type->elementType());
        _temp.append(">");
        _output->insert(_currentOffset, _temp.view());
        _temp.clear();
    }
}

template <bool isOnboard>
void TypeReprGen::writeType(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        writeBuiltin<isOnboard>(type->asBuiltin());
        break;
    case TypeKind::Reference:
        writePointer<isOnboard>(type->asReference());
        break;
    case TypeKind::Array:
        writeArray<isOnboard>(type->asArray());
        break;
    case TypeKind::DynArray:
        writeDynArray<isOnboard>(type->asDynArray());
        break;
    case TypeKind::Function:
        writeFunction<isOnboard>(type->asFunction());
        break;
    case TypeKind::Enum:
        writeNamed<isOnboard>(type->asEnum());
        break;
    case TypeKind::Struct:
        writeNamed<isOnboard>(type->asStruct());
        break;
    case TypeKind::Variant:
        writeNamed<isOnboard>(type->asVariant());
        break;
    case TypeKind::Imported:
        writeNamed<isOnboard>(type->asImported(), type->asImported()->link());
        break;
    case TypeKind::Alias:
        writeNamed<isOnboard>(type->asAlias());
        break;
    case TypeKind::Generic:
        writeNamed<isOnboard>(type->asGeneric()->innerType(), type->asGeneric(), true);
        break;
    case TypeKind::GenericInstantiation:
        writeGenericInstantiation<isOnboard>(type->asGenericInstantiation());
        break;
    case TypeKind::GenericParameter:
        _output->insert(_currentOffset, type->asGenericParemeter()->name());
        break;
    }
}

void TypeReprGen::genOnboardTypeRepr(const Type* type)
{
    genTypeRepr<true>(type);
}

void TypeReprGen::genOnboardTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    genTypeRepr<true>(type, fieldName);
}

void TypeReprGen::genGcTypeRepr(const Type* type)
{
    genTypeRepr<false>(type);
}

void TypeReprGen::genGcTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    genTypeRepr<false>(type, fieldName);
}

template <bool isOnboard>
void TypeReprGen::genTypeRepr(const Type* type)
{
    _currentOffset = _output->size();
    writeType<isOnboard>(type);
}

template <bool isOnboard>
void TypeReprGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _currentOffset = _output->size();
    if (!fieldName.isEmpty()) {
        _output->append(' ');
    }
    _output->append(fieldName);
    writeType<isOnboard>(type);
}
}
