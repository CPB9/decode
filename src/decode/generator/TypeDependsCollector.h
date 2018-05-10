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
class VarRegexp;
class Function;

class TypeDependsCollector : public ConstAstVisitor<TypeDependsCollector> {
public:
    using Depends = HashSet<Rc<const Type>>;

    void collect(const Type* type, Depends* dest);
    void collect(const StatusMsg* msg, Depends* dest);
    void collect(const EventMsg* msg, Depends* dest);
    void collectCmds(Component::Cmds::ConstRange cmds, Depends* dest);
    void collectVars(Component::Vars::ConstRange vars, Depends* dest);
    void collectStatuses(Component::Statuses::ConstRange statuses, Depends* dest);
    void collectEvents(Component::Events::ConstRange events, TypeDependsCollector::Depends* dest);
    void collect(const Component* comp, Depends* dest);
    void collect(const Function* func, Depends* dest);
    void collect(const Ast* ast, Depends* dest);

    bool visitEnumType(const EnumType* enumeration);
    bool visitStructType(const StructType* str);
    bool visitVariantType(const VariantType* variant);
    bool visitImportedType(const ImportedType* u);
    bool visitDynArrayType(const DynArrayType* dynArray);
    bool visitAliasType(const AliasType* alias);
    bool visitFunctionType(const FunctionType* func);
    bool visitGenericType(const GenericType* type);
    bool visitGenericInstantiationType(const GenericInstantiationType* type);

private:
    void collectType(const Type* type);

    const Type* _currentType;
    Depends* _dest;
};

}
