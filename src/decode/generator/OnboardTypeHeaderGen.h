/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/HashSet.h"
#include "decode/generator/TypeDefGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/FuncPrototypeGen.h"
#include <string>

namespace decode {

class Component;
class FunctionType;
class Function;

class OnboardTypeHeaderGen {
public:
    OnboardTypeHeaderGen(SrcBuilder* output);
    ~OnboardTypeHeaderGen();

    void genTypeHeader(const Ast* ast, const TopLevelType* type, bmcl::StringView name);
    void genDynArrayHeader(const DynArrayType* type);
    void genComponentHeader(const Ast* ast, const Component* type);

    void startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName);
    void endIncludeGuard();

private:
    void appendSerializerFuncPrototypes(const Type* type);
    void appendSerializerFuncPrototypes(const Component* comp);

    void startIncludeGuard(const NamedType* type);
    void startIncludeGuard(const TopLevelType* type, bmcl::StringView name);
    void startIncludeGuard(const Component* comp);
    void startIncludeGuard(const DynArrayType* type);

    void appendIncludes(const TypeDependsCollector::Depends& src);
    void appendImplBlockIncludes(const TopLevelType* topLevelType, bmcl::StringView name);
    void appendImplBlockIncludes(const NamedType* topLevelType);
    void appendImplBlockIncludes(const Component* comp);
    void appendImplBlockIncludes(const DynArrayType* dynArray);
    void appendIncludesAndFwds(const Type* topLevelType);
    void appendIncludesAndFwds(const Component* comp);
    void appendCommonIncludePaths();

    void appendStatusStructs(const Component* comp);
    void appendEventStructs(const Component* comp);

    void appendSizeFuncs(const Type* type, bmcl::StringView name, bmcl::StringView prefix, std::size_t size);
    void appendMinMaxSizeFuncs(const Type* type, bmcl::StringView name);
    void appendCompPartSizeFunc(const Component* comp, bmcl::StringView name, bmcl::StringView prefix, std::size_t size);

    void appendFunctionPrototype(const Function* func, bmcl::StringView typeName);
    void appendFunctionPrototypes(const NamedType* type);
    void appendFunctionPrototypes(const TopLevelType* type, bmcl::StringView name);
    void appendImplPrototypes(const Component* comp);
    void appendCommandPrototypes(const Component* comp);
    void appendCommandArgAllocators(const Component* comp);
    void appendCmdEncoderPrototypes(const Component* comp);
    void appendCmdDecoderPrototypes(const Component* comp);
    void appendCmdMinMaxSizeFuncs(const Component* comp);
    void appendStatusEncoderPrototypes(const Component* comp);
    void appendStatusDecoderPrototypes(const Component* comp);
    void appendStatusMinMaxSizeFuncs(const Component* comp);
    void appendEventSenderPrototypes(const Component* comp);
    void appendEventDecoderPrototypes(const Component* comp);
    void appendEventMinMaxSizeFuncs(const Component* comp);
    void appendFunctionPrototypes(RcVec<Function>::ConstRange funcs, bmcl::StringView typeName);

    template <typename T>
    void appendComponentVarDefine(const Component* comp, const T* value, bmcl::StringView sep);

    const Ast* _ast;
    SrcBuilder* _output;
    TypeDependsCollector _includeCollector;
    TypeDefGen _typeDefGen;
    SrcBuilder _dynArrayName;
    FuncPrototypeGen _prototypeGen;
};
}
