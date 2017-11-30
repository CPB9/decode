/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/TypeDependsCollector.h"

#include <string>

namespace decode {

class Component;
class SrcBuilder;
class TypeReprGen;

class TypeDefGen {
public:
    TypeDefGen(SrcBuilder* output);
    ~TypeDefGen();

    void genTypeDef(const TopLevelType* type, bmcl::StringView name);
    void genTypeDef(const DynArrayType* type);
    void genComponentDef(const Component* comp);


private:
    void appendFieldVec(TypeVec::ConstRange fields, bmcl::StringView name);
    void appendFieldVec(FieldVec::ConstRange fields, bmcl::StringView name);
    void appendStruct(const StructType* type, bmcl::StringView name);
    void appendEnum(const EnumType* type, bmcl::StringView name);
    void appendVariant(const VariantType* type, bmcl::StringView name);
    void appendAlias(const AliasType* type, bmcl::StringView name);
    void appendDynArray(const DynArrayType* type);

    SrcBuilder* _output;
};
}
