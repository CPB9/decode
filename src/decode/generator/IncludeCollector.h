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
#include "decode/ast/AstVisitor.h"
#include "decode/ast/Component.h"

#include <string>

namespace decode {

class Type;
class TypeReprGen;
class StatusRegexp;
class Function;

class IncludeCollector : public ConstAstVisitor<IncludeCollector> {
public:
    void collect(const Type* type, HashSet<std::string>* dest);
    void collect(const StatusMsg* msg, HashSet<std::string>* dest);
    void collectCmds(Component::Cmds::ConstRange cmds, HashSet<std::string>* dest);
    void collectParams(Component::Params::ConstRange cmds, HashSet<std::string>* dest);
    void collectStatuses(Component::Statuses::ConstRange statuses, HashSet<std::string>* dest);
    void collect(const Component* comp, HashSet<std::string>* dest);
    void collect(const Function* func, HashSet<std::string>* dest);
    void collect(const Ast* ast, HashSet<std::string>* dest);

    bool visitEnumType(const EnumType* enumeration);
    bool visitStructType(const StructType* str);
    bool visitVariantType(const VariantType* variant);
    bool visitImportedType(const ImportedType* u);
    bool visitDynArrayType(const DynArrayType* dynArray);
    bool visitAliasType(const AliasType* alias);
    bool visitFunctionType(const FunctionType* func);

private:
    void addInclude(const NamedType* type);

    const Type* _currentType;
    HashSet<std::string>* _dest;
};

}
