/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"

#include <string>
#include <unordered_set>

namespace decode {

class Type;
class TypeReprGen;
class StatusRegexp;

class IncludeCollector : public ConstAstVisitor<IncludeCollector> {
public:
    void collect(const Type* type, std::unordered_set<std::string>* dest);
    void collect(const StatusMsg* msg, std::unordered_set<std::string>* dest);
    void collect(const Component* comp, std::unordered_set<std::string>* dest);

    bool visitEnumType(const EnumType* enumeration);
    bool visitStructType(const StructType* str);
    bool visitVariantType(const VariantType* variant);
    bool visitImportedType(const ImportedType* u);
    bool visitSliceType(const SliceType* slice);
    bool visitAliasType(const AliasType* alias);

private:

    void addInclude(const NamedType* type);

    const Type* _currentType;
    std::unordered_set<std::string>* _dest;
};

}
