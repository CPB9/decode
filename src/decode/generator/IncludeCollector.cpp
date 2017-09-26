/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/IncludeCollector.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/core/HashSet.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/Component.h"

namespace decode {

inline bool IncludeCollector::visitEnumType(const EnumType* enumeration)
{
    addInclude(enumeration);
    return true;
}

inline bool IncludeCollector::visitStructType(const StructType* str)
{
    addInclude(str);
    return true;
}

inline bool IncludeCollector::visitVariantType(const VariantType* variant)
{
    addInclude(variant);
    return true;
}

inline bool IncludeCollector::visitImportedType(const ImportedType* u)
{
    addInclude(u->link());
    return false;
}

inline bool IncludeCollector::visitAliasType(const AliasType* alias)
{
    if (alias == _currentType) {
        traverseType(alias->alias()); //HACK
        return false;
    }
    addInclude(alias);
    return false;
}

bool IncludeCollector::visitFunctionType(const FunctionType* func)
{
    for (const Field* arg : func->argumentsRange()) {
        traverseType(arg->type());
    }
    return true;
}

bool IncludeCollector::visitDynArrayType(const DynArrayType* dynArray)
{
    if (dynArray == _currentType) {
        traverseType(dynArray->elementType());
        return false;
    }
    SrcBuilder path;
    path.append("_dynarray_/");
    TypeNameGen gen(&path);
    gen.genTypeName(dynArray);

    _dest->insert(std::move(path.result()));
    ascendTypeOnce(dynArray->elementType());
    return false;
}

bool IncludeCollector::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    if (type == _currentType) {
        traverseType(type->instantiatedType());
        return false;
    }
    SrcBuilder path;
    path.append("_generic_/");
    if (type->moduleName() != "core") {
        path.appendWithFirstUpper(type->moduleName());
    }
    path.append(type->name());
    TypeNameGen gen(&path);
    for (const Type* t : type->substitutedTypesRange()) {
        gen.genTypeName(t);
    }
    _dest->insert(std::move(path.result()));
    return false;
}

void IncludeCollector::addInclude(const NamedType* type)
{
    if (type == _currentType) {
        return;
    }
    std::string path = type->moduleName().toStdString();
    path.push_back('/');
    path.append(type->name().begin(), type->name().end());
    _dest->insert(std::move(path));
}

void IncludeCollector::collect(const StatusMsg* msg, HashSet<std::string>* dest)
{
    _currentType = 0;
    _dest = dest;
    //FIXME: visit only first accessor in every part
    for (const StatusRegexp* part : msg->partsRange()) {
        for (const Accessor* acc : part->accessorsRange()) {
            switch (acc->accessorKind()) {
            case AccessorKind::Field: {
                auto facc = static_cast<const FieldAccessor*>(acc);
                const Type* type = facc->field()->type();
                traverseType(type);
                break;
            }
            case AccessorKind::Subscript: {
                auto sacc = static_cast<const SubscriptAccessor*>(acc);
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

void IncludeCollector::collect(const Type* type, HashSet<std::string>* dest)
{
    _dest = dest;
    _currentType = type;
    traverseType(type);
}

void IncludeCollector::collect(const Component* comp, HashSet<std::string>* dest)
{
    _dest = dest;
    _currentType = 0;
    if (!comp->hasParams()) {
        return;
    }
    for (const Field* field : comp->paramsRange()) {
        traverseType(field->type());
    }
}

void IncludeCollector::collect(const Function* func, HashSet<std::string>* dest)
{
    _dest = dest;
    _currentType = func->type();
    traverseType(func->type());
}

void IncludeCollector::collect(const Ast* ast, HashSet<std::string>* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const Type* t : ast->typesRange()) {
        traverseType(t);
    }
}

void IncludeCollector::collectCmds(Component::Cmds::ConstRange cmds, HashSet<std::string>* dest)
{
    _dest = dest;
    for (const Function* func : cmds) {
        _currentType = func->type();
        traverseType(_currentType);
    }
}

void IncludeCollector::collectParams(Component::Params::ConstRange params, HashSet<std::string>* dest)
{
    _dest = dest;
    _currentType = nullptr;
    for (const Field* param : params) {
        traverseType(param->type());
    }
}

void IncludeCollector::collectStatuses(Component::Statuses::ConstRange statuses, HashSet<std::string>* dest)
{
    for (const StatusMsg* msg : statuses) {
        collect(msg, dest);
    }
}
}

