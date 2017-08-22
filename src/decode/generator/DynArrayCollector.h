/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/ast/AstVisitor.h"
#include "decode/parser/Containers.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class Type;
class TypeReprGen;
class Component;
class DynArrayType;

class DynArrayCollector : public ConstAstVisitor<DynArrayCollector> {
public:
    using NameToDynArrayMap = RcSecondUnorderedMap<std::string, const DynArrayType>;

    DynArrayCollector();
    ~DynArrayCollector();

    void collectUniqueDynArrays(const Type* type, NameToDynArrayMap* dest);
    void collectUniqueDynArrays(const Component* type, NameToDynArrayMap* dest);

    bool visitDynArrayType(const DynArrayType* dynArray);

private:
    SrcBuilder _dynArrayName;
    NameToDynArrayMap* _dest;
};
}
