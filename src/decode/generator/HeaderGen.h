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
#include "decode/generator/IncludeCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/FuncPrototypeGen.h"
#include <string>

namespace decode {

class Component;
class FunctionType;
class Function;

class HeaderGen {
public:
    HeaderGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~HeaderGen();

    void genTypeHeader(const Ast* ast, const Type* type);
    void genSliceHeader(const SliceType* type);
    void genComponentHeader(const Ast* ast, const Component* type);

    void startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName);
    void endIncludeGuard();

private:
    void appendSerializerFuncPrototypes(const Type* type);
    void appendSerializerFuncPrototypes(const Component* comp);

    void startIncludeGuard(const NamedType* type);
    void startIncludeGuard(const Component* comp);
    void startIncludeGuard(const SliceType* type);

    void appendIncludes(const HashSet<std::string>& src);
    void appendImplBlockIncludes(const NamedType* topLevelType);
    void appendImplBlockIncludes(const Component* comp);
    void appendImplBlockIncludes(const SliceType* slice);
    void appendIncludesAndFwds(const Type* topLevelType);
    void appendIncludesAndFwds(const Component* comp);
    void appendCommonIncludePaths();

    void appendFunctionPrototype(const Function* func, bmcl::StringView typeName);
    void appendFunctionPrototypes(const NamedType* type);
    void appendFunctionPrototypes(const Component* comp);
    void appendCommandPrototypes(const Component* comp);
    void appendFunctionPrototypes(RcVec<Function>::ConstRange funcs, bmcl::StringView typeName);

    const Ast* _ast;
    SrcBuilder* _output;
    IncludeCollector _includeCollector;
    TypeDefGen _typeDefGen;
    SrcBuilder _sliceName;
    FuncPrototypeGen _prototypeGen;
    Rc<TypeReprGen> _typeReprGen;
};
}
