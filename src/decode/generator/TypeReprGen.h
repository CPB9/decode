/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"

namespace decode {

class Type;
class ArrayType;
class BuiltinType;
class ReferenceType;
class FunctionType;
class DynArrayType;
class NamedType;
class GenericInstantiationType;

class TypeReprGen : public RefCountable {
public:
    using Pointer = Rc<TypeReprGen>;
    using ConstPointer = Rc<const TypeReprGen>;

    TypeReprGen(SrcBuilder* dest);
    ~TypeReprGen();

    void genOnboardTypeRepr(const Type* type);
    void genOnboardTypeRepr(const Type* type, bmcl::StringView fieldName);

    void genGcTypeRepr(const Type* type);
    void genGcTypeRepr(const Type* type, bmcl::StringView fieldName);

private:
    template <bool isOnboard>
    void genTypeRepr(const Type* type);
    template <bool isOnboard>
    void genTypeRepr(const Type* type, bmcl::StringView fieldName);
    template <bool isOnboard>
    void writeBuiltin(const BuiltinType* type);
    template <bool isOnboard>
    void writeArray(const ArrayType* type);
    template <bool isOnboard>
    void writePointer(const ReferenceType* type);
    template <bool isOnboard>
    void writeNamed(const NamedType* type);
    template <bool isOnboard>
    void writeNamed(const NamedType* type, const NamedType* origin, bool originIsGeneric = false);
    template <bool isOnboard>
    void writeFunction(const FunctionType* type);
    template <bool isOnboard>
    void writeGenericInstantiation(const GenericInstantiationType* type);
    template <bool isOnboard>
    void writeDynArray(const DynArrayType* type);
    template <bool isOnboard>
    void writeType(const Type* type);

    void writeOnboardTypeName(const Type* type);

    SrcBuilder* _output;
    std::size_t _currentOffset;
    SrcBuilder _temp;
};
}
