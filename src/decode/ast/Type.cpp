/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Type.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Field.h"

#include <bmcl/Logging.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/Option.h>

namespace decode {

Type::Type(TypeKind kind)
    : _typeKind(kind)
{
}

Type::~Type()
{
}

bool Type::isArray() const
{
    return _typeKind == TypeKind::Array;
}

bool Type::isDynArray() const
{
    return _typeKind == TypeKind::DynArray;
}

bool Type::isStruct() const
{
    return _typeKind == TypeKind::Struct;
}

bool Type::isFunction() const
{
    return _typeKind == TypeKind::Function;
}

bool Type::isBuiltin() const
{
    return _typeKind == TypeKind::Builtin;
}

bool Type::isAlias() const
{
    return _typeKind == TypeKind::Alias;
}

bool Type::isImported() const
{
    return _typeKind == TypeKind::Imported;
}

bool Type::isVariant() const
{
    return _typeKind == TypeKind::Variant;
}

bool Type::isEnum() const
{
    return _typeKind == TypeKind::Enum;
}

bool Type::isReference() const
{
    return _typeKind == TypeKind::Reference;
}

bool Type::isBuiltinChar() const
{
    if (isBuiltin()) {
        return asBuiltin()->builtinTypeKind() == BuiltinTypeKind::Char;
    }
    return false;
}

TypeKind Type::typeKind() const
{
    return _typeKind;
}

const ArrayType* Type::asArray() const
{
    assert(isArray());
    return static_cast<const ArrayType*>(this);
}

const DynArrayType* Type::asDynArray() const
{
    assert(isDynArray());
    return static_cast<const DynArrayType*>(this);
}

const StructType* Type::asStruct() const
{
    assert(isStruct());
    return static_cast<const StructType*>(this);
}

const FunctionType* Type::asFunction() const
{
    assert(isFunction());
    return static_cast<const FunctionType*>(this);
}

const BuiltinType* Type::asBuiltin() const
{
    assert(isBuiltin());
    return static_cast<const BuiltinType*>(this);
}

const AliasType* Type::asAlias() const
{
    assert(isAlias());
    return static_cast<const AliasType*>(this);
}

const ImportedType* Type::asImported() const
{
    assert(isImported());
    return static_cast<const ImportedType*>(this);
}

const VariantType* Type::asVariant() const
{
    assert(isVariant());
    return static_cast<const VariantType*>(this);
}

const EnumType* Type::asEnum() const
{
    assert(isEnum());
    return static_cast<const EnumType*>(this);
}

const ReferenceType* Type::asReference() const
{
    assert(isReference());
    return static_cast<const ReferenceType*>(this);
}

ArrayType* Type::asArray()
{
    assert(isArray());
    return static_cast<ArrayType*>(this);
}

DynArrayType* Type::asDynArray()
{
    assert(isDynArray());
    return static_cast<DynArrayType*>(this);
}

StructType* Type::asStruct()
{
    assert(isStruct());
    return static_cast<StructType*>(this);
}

FunctionType* Type::asFunction()
{
    assert(isFunction());
    return static_cast<FunctionType*>(this);
}

BuiltinType* Type::asBuiltin()
{
    assert(isBuiltin());
    return static_cast<BuiltinType*>(this);
}

AliasType* Type::asAlias()
{
    assert(isAlias());
    return static_cast<AliasType*>(this);
}

ImportedType* Type::asImported()
{
    assert(isImported());
    return static_cast<ImportedType*>(this);
}

VariantType* Type::asVariant()
{
    assert(isVariant());
    return static_cast<VariantType*>(this);
}

EnumType* Type::asEnum()
{
    assert(isEnum());
    return static_cast<EnumType*>(this);
}

ReferenceType* Type::asReference()
{
    assert(isReference());
    return static_cast<ReferenceType*>(this);
}

const Type* Type::resolveFinalType() const
{
    if (_typeKind == TypeKind::Alias) {
        return asAlias()->alias()->resolveFinalType();
    }
    if (_typeKind == TypeKind::Imported) {
        return asImported()->link()->resolveFinalType();
    }
    return this;
}

template <typename R, typename C>
bool compareRanges(R r1, R r2, C&& comparator)
{
    if (r1.size() != r2.size()) {
        return false;
    }
    auto i1 = r1.begin();
    auto i2 = r2.begin();
    for (; i1 != r1.end(); i1++, i2++) {
        if (!comparator(*i1, *i2)) {
            return false;
        }
    }
    return true;
}

bool Type::equals(const Type* other) const
{
    const Type* first = resolveFinalType();
    const Type* second = other->resolveFinalType();
    if (first == second) {
        return true;
    }
    if (first->typeKind() != second->typeKind()) {
        return false;
    }

    switch (first->typeKind()) {
        case TypeKind::Builtin:
            return first->asBuiltin()->builtinTypeKind() == second->asBuiltin()->builtinTypeKind();
        case TypeKind::Reference: {
            const ReferenceType* l = first->asReference();
            const ReferenceType* r = second->asReference();
            return (l->isMutable() == r->isMutable()) &&
                   (l->referenceKind() == r->referenceKind()) &&
                   (l->pointee()->equals(r->pointee()));
        }
        case TypeKind::Array: {
            const ArrayType* l = first->asArray();
            const ArrayType* r = second->asArray();
            return (l->elementCount() == r->elementCount()) &&
                   (l->elementType()->equals(r->elementType()));
        }
        case TypeKind::DynArray:
            return first->asDynArray()->elementType()->equals(second->asDynArray()->elementType());
        case TypeKind::Function: {
            const FunctionType* l = first->asFunction();
            const FunctionType* r = second->asFunction();
            if (l->selfArgument() != r->selfArgument()) {
                return false;
            }
            if (l->hasReturnValue()) {
                if (r->hasReturnValue()) {
                    if (!l->returnValue().unwrap()->equals(r->returnValue().unwrap())) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            return compareRanges(l->argumentsRange(), r->argumentsRange(), [](const Field* lt, const Field* rt) {
                if (lt->name() != rt->name()) {
                    return false;
                }
                return lt->type()->equals(rt->type());
            });
            return true;
        }
        case TypeKind::Enum: {
            const EnumType* l = first->asEnum();
            const EnumType* r = second->asEnum();
            if (l->name() != r->name()) {
                return false;
            }
            return compareRanges(l->constantsRange(), r->constantsRange(), [](const EnumConstant* lt, const EnumConstant* rt) {
                if (lt->isUserSet() != rt->isUserSet()) {
                    return false;
                }
                return lt->value() == rt->value();
            });
        }
        case TypeKind::Struct: {
            const StructType* l = first->asStruct();
            const StructType* r = second->asStruct();
            if (l->name() != r->name()) {
                return false;
            }
            return compareRanges(l->fieldsRange(), r->fieldsRange(), [](const Field* lt, const Field* rt) {
                if (lt->name() != rt->name()) {
                    return false;
                }
                return lt->type()->equals(rt->type());
            });
        }
        case TypeKind::Variant: {
            const VariantType* l = first->asVariant();
            const VariantType* r = second->asVariant();
            if (l->name() != r->name()) {
                return false;
            }
            return compareRanges(l->fieldsRange(), r->fieldsRange(), [](const VariantField* lt, const VariantField* rt) {
                if (lt->name() != rt->name()) {
                    return false;
                }
                return true;
            });
        }
        case TypeKind::Imported:
            assert(false);
            return false;
        case TypeKind::Alias:
            assert(false);
            return false;
    }

    return true;
}

NamedType::NamedType(TypeKind kind, bmcl::StringView name, const ModuleInfo* info)
    : Type(kind)
    , _name(name)
    , _moduleInfo(info)
{
}

NamedType::~NamedType()
{
}

const ModuleInfo* NamedType::moduleInfo() const
{
    return _moduleInfo.get();
}

bmcl::StringView NamedType::moduleName() const
{
    return _moduleInfo->moduleName();
}

bmcl::StringView NamedType::name() const
{
    return _name;
}

void NamedType::setName(bmcl::StringView name)
{
    _name = name;
}

void NamedType::setModuleInfo(const ModuleInfo* info)
{
    _moduleInfo.reset(info);
}

AliasType::AliasType(bmcl::StringView name, const ModuleInfo* info, Type* alias)
    : NamedType(TypeKind::Alias, name, info)
    , _alias(alias)
{
}

AliasType::~AliasType()
{
}

const Type* AliasType::alias() const
{
    return _alias.get();
}

Type* AliasType::alias()
{
    return _alias.get();
}

void AliasType::setAlias(AliasType* type)
{
    _alias.reset(type);
}

ReferenceType::ReferenceType(ReferenceKind kind, bool isMutable, Type* pointee)
    : Type(TypeKind::Reference)
    , _pointee(pointee)
    , _referenceKind(kind)
    , _isMutable(isMutable)
{
}

ReferenceType::~ReferenceType()
{
}

bool ReferenceType::isMutable() const
{
    return _isMutable;
}

ReferenceKind ReferenceType::referenceKind() const
{
    return _referenceKind;
}

const Type* ReferenceType::pointee() const
{
    return _pointee.get();
}

Type* ReferenceType::pointee()
{
    return _pointee.get();
}

void ReferenceType::setPointee(Type* pointee)
{
    _pointee.reset(pointee);
}

void ReferenceType::setMutable(bool isMutable)
{
    _isMutable = isMutable;
}

void ReferenceType::setReferenceKind(ReferenceKind kind)
{
    _referenceKind = kind;
}

BuiltinType::BuiltinType(BuiltinTypeKind kind)
    : Type(TypeKind::Builtin)
    , _builtinTypeKind(kind)
{
}

BuiltinType::~BuiltinType()
{
}

BuiltinTypeKind BuiltinType::builtinTypeKind() const
{
    return _builtinTypeKind;
}

DynArrayType::DynArrayType(const ModuleInfo* info, std::uintmax_t maxSize, Type* elementType)
    : Type(TypeKind::DynArray)
    , _moduleInfo(info)
    , _maxSize(maxSize)
    , _elementType(elementType)
{
}

DynArrayType::~DynArrayType()
{
}

const Type* DynArrayType::elementType() const
{
    return _elementType.get();
}

Type* DynArrayType::elementType()
{
    return _elementType.get();
}

std::uintmax_t DynArrayType::maxSize() const
{
    return _maxSize;
}

const ModuleInfo* DynArrayType::moduleInfo() const
{
    return _moduleInfo.get();
}

bmcl::StringView DynArrayType::moduleName() const
{
    return _moduleInfo->moduleName();
}

ArrayType::ArrayType(std::uintmax_t elementCount, Type* elementType)
    : Type(TypeKind::Array)
    , _elementCount(elementCount)
    , _elementType(elementType)
{
}

ArrayType::~ArrayType()
{
}

std::uintmax_t ArrayType::elementCount() const
{
    return _elementCount;
}

const Type* ArrayType::elementType() const
{
    return _elementType.get();
}

Type* ArrayType::elementType()
{
    return _elementType.get();
}

ImportedType::ImportedType(bmcl::StringView name, bmcl::StringView importPath, const ModuleInfo* info, NamedType* link)
    : NamedType(TypeKind::Imported, name, info)
    , _importPath(importPath)
    , _link(link)
{
}

ImportedType::~ImportedType()
{
}

const NamedType* ImportedType::link() const
{
    return _link.get();
}

NamedType* ImportedType::link()
{
    return _link.get();
}

void ImportedType::setLink(NamedType* link)
{
    _link.reset(link);
}

FunctionType::FunctionType(const ModuleInfo* info)
    : Type(TypeKind::Function)
    , _modInfo(info)
{
}

FunctionType::~FunctionType()
{
}

bmcl::OptionPtr<Type> FunctionType::returnValue()
{
    return _returnValue.get();
}

bmcl::OptionPtr<const Type> FunctionType::returnValue() const
{
    return _returnValue.get();
}

bool FunctionType::hasReturnValue() const
{
    return _returnValue.get() != nullptr;
}

bool FunctionType::hasArguments() const
{
    return !_arguments.empty();
}

FieldVec::Iterator FunctionType::argumentsBegin()
{
    return _arguments.begin();
}

FieldVec::Iterator FunctionType::argumentsEnd()
{
    return _arguments.end();
}

FieldVec::Range FunctionType::argumentsRange()
{
    return _arguments;
}

FieldVec::ConstIterator FunctionType::argumentsBegin() const
{
    return _arguments.cbegin();
}

FieldVec::ConstIterator FunctionType::argumentsEnd() const
{
    return _arguments.cend();
}

FieldVec::ConstRange FunctionType::argumentsRange() const
{
    return _arguments;
}

bmcl::Option<SelfArgument> FunctionType::selfArgument() const
{
    return _self;
}

void FunctionType::addArgument(Field* field)
{
    _arguments.emplace_back(field);
}

void FunctionType::setReturnValue(Type* type)
{
    _returnValue.reset(type);
}

void FunctionType::setSelfArgument(SelfArgument arg)
{
    _self.emplace(arg);
}

StructType::StructType(bmcl::StringView name, const ModuleInfo* info)
    : NamedType(TypeKind::Struct, name, info)
{
}

StructType::~StructType()
{
}

StructType::Fields::ConstIterator StructType::fieldsBegin() const
{
    return _fields.cbegin();
}

StructType::Fields::ConstIterator StructType::fieldsEnd() const
{
    return _fields.cend();
}

StructType::Fields::ConstRange StructType::fieldsRange() const
{
    return _fields;
}

StructType::Fields::Iterator StructType::fieldsBegin()
{
    return _fields.begin();
}

StructType::Fields::Iterator StructType::fieldsEnd()
{
    return _fields.end();
}

StructType::Fields::Range StructType::fieldsRange()
{
    return _fields;
}

void StructType::addField(Field* field)
{
    _fields.emplace_back(field);
    _nameToFieldMap.emplace(field->name(), field);
}

const Field* StructType::fieldAt(std::size_t index) const
{
    return _fields[index].get();
}

bmcl::OptionPtr<const Field> StructType::fieldWithName(bmcl::StringView name) const
{
    return _nameToFieldMap.findValueWithKey(name);
}

bmcl::Option<std::size_t> StructType::indexOfField(const Field* field) const
{
    auto it = std::find(_fields.begin(), _fields.end(), field);
    if (it == _fields.end()) {
        return bmcl::None;
    }
    return std::distance(_fields.begin(), it);
}

EnumConstant::EnumConstant(bmcl::StringView name, std::int64_t value, bool isUserSet)
    : NamedRc(name)
    , _value(value)
    , _isUserSet(isUserSet)
{
}

EnumConstant::~EnumConstant()
{
}

std::int64_t EnumConstant::value() const
{
    return _value;
}

bool EnumConstant::isUserSet() const
{
    return _isUserSet;
}

EnumType::EnumType(bmcl::StringView name, const ModuleInfo* info)
    : NamedType(TypeKind::Enum, name, info)
{
}

EnumType::~EnumType()
{
}

EnumType::Constants::ConstIterator EnumType::constantsBegin() const
{
    return _constantDecls.cbegin();
}

EnumType::Constants::ConstIterator EnumType::constantsEnd() const
{
    return _constantDecls.cend();
}

EnumType::Constants::ConstRange EnumType::constantsRange() const
{
    return _constantDecls;
}

bool EnumType::addConstant(EnumConstant* constant)
{
    auto pair = _constantDecls.emplace(constant->value(), constant);
    return pair.second;
}

VariantType::VariantType(bmcl::StringView name, const ModuleInfo* info)
    : NamedType(TypeKind::Variant, name, info)
{
}

VariantType::~VariantType()
{
}

VariantType::Fields::ConstIterator VariantType::fieldsBegin() const
{
    return _fields.cbegin();
}

VariantType::Fields::ConstIterator VariantType::fieldsEnd() const
{
    return _fields.cend();
}

VariantType::Fields::ConstRange VariantType::fieldsRange() const
{
    return _fields;
}

VariantType::Fields::Iterator VariantType::fieldsBegin()
{
    return _fields.begin();
}

VariantType::Fields::Iterator VariantType::fieldsEnd()
{
    return _fields.end();
}

VariantType::Fields::Range VariantType::fieldsRange()
{
    return _fields;
}

void VariantType::addField(VariantField* field)
{
    _fields.emplace_back(field);
}
}
