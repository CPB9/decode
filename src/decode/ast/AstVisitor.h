/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/ast/Decl.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"

namespace decode {

//TODO: traverse impl functions

template<class T> struct Ptr { typedef T* type; };
template<class T> struct ConstPtr { typedef const T* type; };

template <template <typename> class P, typename T, typename R>
typename P<T>::type ptrCast(const R* type)
{
    return static_cast<typename P<T>::type>(type);
}

template <template <typename> class P, typename T, typename R>
typename P<T>::type ptrCast(R* type)
{
    return static_cast<typename P<T>::type>(type);
}

template <typename B, template <typename> class P>
class AstVisitorBase {
public:
    void traverseType(typename P<Type>::type type);
    void traverseComponentParameters(typename P<Component>::type comp);
    void traverseComponentCommands(typename P<Component>::type comp);

    void traverseBuiltinType(typename P<BuiltinType>::type builtin);
    void traverseArrayType(typename P<ArrayType>::type array);
    void traverseDynArrayType(typename P<DynArrayType>::type dynArray);
    void traverseReferenceType(typename P<ReferenceType>::type ref);
    void traverseFunctionType(typename P<FunctionType>::type fn);
    void traverseEnumType(typename P<EnumType>::type enumeration);
    void traverseStructType(typename P<StructType>::type str);
    void traverseVariantType(typename P<VariantType>::type variant);
    void traverseImportedType(typename P<ImportedType>::type u);
    void traverseAliasType(typename P<AliasType>::type alias);
    void traverseGenericType(typename P<GenericType>::type generic);
    void traverseGenericInstantiationType(typename P<GenericInstantiationType>::type generic);
    void traverseGenericParameterType(typename P<GenericParameterType>::type generic);

    void traverseVariantField(typename P<VariantField>::type field);
    void traverseConstantVariantField(typename P<ConstantVariantField>::type field);
    void traverseTupleVariantField(typename P<TupleVariantField>::type field);
    void traverseStructVariantField(typename P<StructVariantField>::type field);

    B& base();

    constexpr bool shouldFollowImportedType() const;

protected:
    void ascendTypeOnce(typename P<Type>::type type);

    bool visitType(typename P<Type>::type type);
    bool visitBuiltinType(typename P<BuiltinType>::type builtin);
    bool visitArrayType(typename P<ArrayType>::type array);
    bool visitDynArrayType(typename P<DynArrayType>::type dynArray);
    bool visitReferenceType(typename P<ReferenceType>::type ref);
    bool visitFunctionType(typename P<FunctionType>::type fn);
    bool visitEnumType(typename P<EnumType>::type enumeration);
    bool visitStructType(typename P<StructType>::type str);
    bool visitVariantType(typename P<VariantType>::type variant);
    bool visitImportedType(typename P<ImportedType>::type u);
    bool visitAliasType(typename P<AliasType>::type alias);
    bool visitGenericType(typename P<GenericType>::type generic);
    bool visitGenericInstantiationType(typename P<GenericInstantiationType>::type generic);
    bool visitGenericParameterType(typename P<GenericParameterType>::type generic);

    bool visitVariantField(typename P<VariantField>::type field);
    bool visitConstantVariantField(typename P<ConstantVariantField>::type field);
    bool visitTupleVariantField(typename P<TupleVariantField>::type field);
    bool visitStructVariantField(typename P<StructVariantField>::type field);
};

template <typename B, template <typename> class P>
inline B& AstVisitorBase<B, P>::base()
{
    return *static_cast<B*>(this);
}

template <typename B, template <typename> class P>
constexpr bool AstVisitorBase<B, P>::shouldFollowImportedType() const
{
    return false;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitType(typename P<Type>::type type)
{
    (void)type;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitFunctionType(typename P<FunctionType>::type fn)
{
    (void)fn;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitBuiltinType(typename P<BuiltinType>::type builtin)
{
    (void)builtin;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitArrayType(typename P<ArrayType>::type array)
{
    (void)array;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitDynArrayType(typename P<DynArrayType>::type dynArray)
{
    (void)dynArray;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitEnumType(typename P<EnumType>::type enumeration)
{
    (void)enumeration;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitStructType(typename P<StructType>::type str)
{
    (void)str;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitVariantType(typename P<VariantType>::type variant)
{
    (void)variant;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitReferenceType(typename P<ReferenceType>::type ref)
{
    (void)ref;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitImportedType(typename P<ImportedType>::type u)
{
    (void)u;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitAliasType(typename P<AliasType>::type alias)
{
    (void)alias;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitGenericType(typename P<GenericType>::type generic)
{
    (void)generic;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitGenericInstantiationType(typename P<GenericInstantiationType>::type generic)
{
    (void)generic;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitGenericParameterType(typename P<GenericParameterType>::type generic)
{
    (void)generic;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitVariantField(typename P<VariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitConstantVariantField(typename P<ConstantVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitTupleVariantField(typename P<TupleVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitStructVariantField(typename P<StructVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::ascendTypeOnce(typename P<Type>::type type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        base().visitBuiltinType(builtin);
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        base().visitReferenceType(ref);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        base().visitArrayType(array);
        break;
    }
    case TypeKind::DynArray: {
        typename P<DynArrayType>::type ref = ptrCast<P, DynArrayType>(type);
        base().visitDynArrayType(ref);
        break;
    }
    case TypeKind::Function: {
        typename P<FunctionType>::type fn = ptrCast<P, FunctionType>(type);
        base().visitFunctionType(fn);
        break;
    }
    case TypeKind::Enum: {
        typename P<EnumType>::type en = ptrCast<P, EnumType>(type);
        base().visitEnumType(en);
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        base().visitStructType(str);
        break;
    }
    case TypeKind::Variant: {
        typename P<VariantType>::type variant = ptrCast<P, VariantType>(type);
        base().visitVariantType(variant);
        break;
    }
    case TypeKind::Imported: {
        typename P<ImportedType>::type u = ptrCast<P, ImportedType>(type);
        base().visitImportedType(u);
        break;
    }
    case TypeKind::Alias: {
        typename P<AliasType>::type alias = ptrCast<P, AliasType>(type);
        base().visitAliasType(alias);
        break;
    }
    case TypeKind::Generic: {
        typename P<GenericType>::type generic = ptrCast<P, GenericType>(type);
        base().visitGenericType(generic);
        break;
    }
    case TypeKind::GenericInstantiation: {
        typename P<GenericInstantiationType>::type generic = ptrCast<P, GenericInstantiationType>(type);
        base().visitGenericInstantiationType(generic);
        break;
    }
    case TypeKind::GenericParameter: {
        typename P<GenericParameterType>::type generic = ptrCast<P, GenericParameterType>(type);
        base().visitGenericParameterType(generic);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseBuiltinType(typename P<BuiltinType>::type builtin)
{
    if (!base().visitBuiltinType(builtin)) {
        return;
    }
    //TODO: visit builtin types
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseReferenceType(typename P<ReferenceType>::type ref)
{
    if (!base().visitReferenceType(ref)) {
        return;
    }
    traverseType(ref->pointee());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseArrayType(typename P<ArrayType>::type array)
{
    if (!base().visitArrayType(array)) {
        return;
    }
    traverseType(array->elementType());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseDynArrayType(typename P<DynArrayType>::type dynArray)
{
    if (!base().visitDynArrayType(dynArray)) {
        return;
    }
    traverseType(dynArray->elementType());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseFunctionType(typename P<FunctionType>::type fn)
{
    if (!base().visitFunctionType(fn)) {
        return;
    }
    auto rv = fn->returnValue();
    if (rv.isSome()) {
        traverseType(rv.unwrap());
    }
    for (typename P<Field>::type field : fn->argumentsRange()) {
        traverseType(field->type());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseEnumType(typename P<EnumType>::type en)
{
    if (!base().visitEnumType(en)) {
        return;
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseStructType(typename P<StructType>::type str)
{
    if (!base().visitStructType(str)) {
        return;
    }
    for (typename P<Field>::type field : str->fieldsRange()) {
        traverseType(field->type());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseConstantVariantField(typename P<ConstantVariantField>::type field)
{
    if (!base().visitConstantVariantField(field)) {
        return;
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseTupleVariantField(typename P<TupleVariantField>::type field)
{
    if (!base().visitTupleVariantField(field)) {
        return;
    }
    for (typename P<Type>::type t : field->typesRange()) {
        traverseType(t);
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseStructVariantField(typename P<StructVariantField>::type sfield)
{
    if (!base().visitStructVariantField(sfield)) {
        return;
    }
    for (typename P<Field>::type field : sfield->fieldsRange()) {
        traverseType(field->type());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseVariantField(typename P<VariantField>::type field)
{
    if (!base().visitVariantField(field)) {
        return;
    }
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant: {
        typename P<ConstantVariantField>::type f = ptrCast<P, ConstantVariantField>(field);
        traverseConstantVariantField(f);
        break;
    }
    case VariantFieldKind::Tuple: {
        typename P<TupleVariantField>::type f = ptrCast<P, TupleVariantField>(field);
        traverseTupleVariantField(f);
        break;
    }
    case VariantFieldKind::Struct: {
        typename P<StructVariantField>::type f = ptrCast<P, StructVariantField>(field);
        traverseStructVariantField(f);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseVariantType(typename P<VariantType>::type variant)
{
    if (!base().visitVariantType(variant)) {
        return;
    }
    for (typename P<VariantField>::type field : variant->fieldsRange()) {
        traverseVariantField(field);
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseImportedType(typename P<ImportedType>::type u)
{
    if (!base().visitImportedType(u)) {
        return;
    }
    if (base().shouldFollowImportedType()) {
        traverseType(u->link());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseAliasType(typename P<AliasType>::type alias)
{
    if (!base().visitAliasType(alias)) {
        return;
    }
    traverseType(alias->alias());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseGenericType(typename P<GenericType>::type generic)
{
    if(!base().visitGenericType(generic)) {
        return;
    }
    //TODO: visit parameters
    traverseType(generic->innerType());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseGenericInstantiationType(typename P<GenericInstantiationType>::type generic)
{
    if(!base().visitGenericInstantiationType(generic)) {
        return;
    }
    //NOTE: no need to visit parameters
    traverseType(generic->instantiatedType());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseGenericParameterType(typename P<GenericParameterType>::type generic)
{
    base().visitGenericParameterType(generic);
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseType(typename P<Type>::type type)
{
    if (!base().visitType(type)) {
        return;
    }
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        traverseBuiltinType(builtin);
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        traverseReferenceType(ref);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        traverseArrayType(array);
        break;
    }
    case TypeKind::DynArray: {
        typename P<DynArrayType>::type dynArray = ptrCast<P, DynArrayType>(type);
        traverseDynArrayType(dynArray);
        break;
    }
    case TypeKind::Function: {
        typename P<FunctionType>::type fn = ptrCast<P, FunctionType>(type);
        traverseFunctionType(fn);
        break;
    }
    case TypeKind::Enum: {
        typename P<EnumType>::type en = ptrCast<P, EnumType>(type);
        traverseEnumType(en);
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        traverseStructType(str);
        break;
    }
    case TypeKind::Variant: {
        typename P<VariantType>::type variant = ptrCast<P, VariantType>(type);
        traverseVariantType(variant);
        break;
    }
    case TypeKind::Imported: {
        typename P<ImportedType>::type u = ptrCast<P, ImportedType>(type);
        traverseImportedType(u);
        break;
    }
    case TypeKind::Alias: {
        typename P<AliasType>::type alias = ptrCast<P, AliasType>(type);
        traverseAliasType(alias);
        break;
    }
    case TypeKind::Generic: {
        typename P<GenericType>::type generic = ptrCast<P, GenericType>(type);
        traverseGenericType(generic);
        break;
    }
    case TypeKind::GenericInstantiation: {
        typename P<GenericInstantiationType>::type generic = ptrCast<P, GenericInstantiationType>(type);
        traverseGenericInstantiationType(generic);
        break;
    }
    case TypeKind::GenericParameter: {
        typename P<GenericParameterType>::type generic = ptrCast<P, GenericParameterType>(type);
        traverseGenericParameterType(generic);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseComponentParameters(typename P<Component>::type comp)
{
    if (comp->hasVars()) {
        for (typename P<Field>::type field : comp->varsRange()) {
            traverseType(field->type());
        }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseComponentCommands(typename P<Component>::type comp)
{
    if (comp->hasCmds()) {
        for (typename P<Function>::type func : comp->cmdsRange()) {
            traverseType(func->type());
        }
    }
}

template <typename B>
using AstVisitor = AstVisitorBase<B, Ptr>;

template <typename B>
using ConstAstVisitor = AstVisitorBase<B, ConstPtr>;
}
