/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/core/HashSet.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/Component.h"

namespace decode {

void TypeDependsCollector::collectType(const Type* type)
{
    if (type != _currentType) {
        _dest->emplace(type);
    }
}

inline bool TypeDependsCollector::visitEnumType(const EnumType* enumeration)
{
    collectType(enumeration);
    return true;
}

inline bool TypeDependsCollector::visitStructType(const StructType* str)
{
    collectType(str);
    return true;
}

inline bool TypeDependsCollector::visitVariantType(const VariantType* variant)
{
    collectType(variant);
    return true;
}

inline bool TypeDependsCollector::visitImportedType(const ImportedType* u)
{
    collectType(u->link());
    return false;
}

inline bool TypeDependsCollector::visitAliasType(const AliasType* alias)
{
    collectType(alias);
    return true;
}

bool TypeDependsCollector::visitFunctionType(const FunctionType* func)
{
    for (const Field* arg : func->argumentsRange()) {
        traverseType(arg->type());
    }
    return true;
}

bool TypeDependsCollector::visitGenericType(const GenericType* type)
{
    collectType(type);
    return false;
}

bool TypeDependsCollector::visitDynArrayType(const DynArrayType* dynArray)
{
    if (dynArray == _currentType) {
        traverseType(dynArray->elementType());
        return false;
    }
    _dest->insert(dynArray);
    traverseType(dynArray->elementType());
    return false;
}

bool TypeDependsCollector::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    collectType(type->genericType());
    if (type == _currentType) {
        _currentType = type->instantiatedType();
        traverseType(_currentType);
        return false;
    }

    for (const Type* t : type->substitutedTypesRange()) {
        traverseType(t);
    }
    collectType(type);
    return false;
}

void TypeDependsCollector::collect(const EventMsg* msg, Depends* dest)
{
    _currentType = 0;
    _dest = dest;
    for (const Field* field : msg->partsRange()) {
        traverseType(field->type());
    };
}

void TypeDependsCollector::collect(const StatusMsg* msg, TypeDependsCollector::Depends* dest)
{
    _currentType = 0;
    _dest = dest;
    //FIXME: visit only first accessor in every part
    for (const VarRegexp* part : msg->partsRange()) {
        traverseType(part->type());
        for (const Accessor* acc : part->accessorsRange()) {
            switch (acc->accessorKind()) {
            case AccessorKind::Field: {
                auto facc = acc->asFieldAccessor();
                const Type* type = facc->field()->type();
                traverseType(type);
                break;
            }
            case AccessorKind::Subscript: {
                auto sacc = acc->asSubscriptAccessor();
                const Type* type = sacc->type();
                traverseType(type);
                break;
            }
            default:
                assert(false);
            }
        }
    }
}

void TypeDependsCollector::collect(const Type* type, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = type;
    traverseType(type);
}

void TypeDependsCollector::collect(const Component* comp, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = 0;
    if (!comp->hasVars()) {
        return;
    }
    for (const Field* field : comp->varsRange()) {
        traverseType(field->type());
    }
}

void TypeDependsCollector::collect(const Function* func, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = func->type();
    traverseType(func->type());
}

void TypeDependsCollector::collect(const Ast* ast, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const Type* t : ast->typesRange()) {
        traverseType(t);
    }
}

void TypeDependsCollector::collectCmds(Component::Cmds::ConstRange cmds, TypeDependsCollector::Depends* dest)
{
    _currentType = 0;
    _dest = dest;
    for (const Function* func : cmds) {
        _currentType = func->type();
        traverseType(_currentType);
    }
}

void TypeDependsCollector::collectVars(Component::Vars::ConstRange vars, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const Field* var : vars) {
        traverseType(var->type());
    }
}

void TypeDependsCollector::collectStatuses(Component::Statuses::ConstRange statuses, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const StatusMsg* msg : statuses) {
        collect(msg, dest);
    }
}

void TypeDependsCollector::collectEvents(Component::Events::ConstRange events, TypeDependsCollector::Depends* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const EventMsg* msg : events) {
        collect(msg, dest);
    }
}
}

