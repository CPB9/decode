/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/FuncPrototypeGen.h"
#include "decode/generator/InlineTypeInspector.h"

#include <bmcl/StringView.h>

#include <string>

namespace decode {

class SrcBuilder;
class TypeReprGen;
class DynArrayType;
class EnumType;
class StructType;
class VariantType;
class NamedType;
class DynArrayType;
class GenericInstantiationType;

class OnboardTypeSourceGen {
public:
    OnboardTypeSourceGen(SrcBuilder* output);
    ~OnboardTypeSourceGen();

    void genTypeSource(const NamedType* type, bmcl::StringView name);
    void genTypeSource(const GenericInstantiationType* type, bmcl::StringView name);
    void genTypeSource(const DynArrayType* type);

private:
    template <typename T, typename F>
    void genSource(const T* type, F&& serGen, F&& deserGen);

    void genSource(const Type* type, bmcl::StringView modName);

    bool visitDynArrayType(const DynArrayType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);

    void appendEnumSerializer(const EnumType* type);
    void appendEnumDeserializer(const EnumType* type);
    void appendStructSerializer(const StructType* type);
    void appendStructDeserializer(const StructType* type);
    void appendVariantSerializer(const VariantType* type);
    void appendVariantDeserializer(const VariantType* type);
    void appendDynArraySerializer(const DynArrayType* type);
    void appendDynArrayDeserializer(const DynArrayType* type);

    void appendIncludes(bmcl::StringView modName);

    SrcBuilder* _output;
    InlineTypeInspector _inlineInspector;
    FuncPrototypeGen _prototypeGen;
    bmcl::StringView _name;
    bmcl::StringView _fileName;
    const Type* _baseType;
};

}
