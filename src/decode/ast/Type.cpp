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
#include "decode/core/EncodedSizes.h"

#include <bmcl/Logging.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/Option.h>
#include <bmcl/Result.h>
#include <bmcl/Panic.h>
#include <bmcl/ZigZag.h>
#include <bmcl/Varuint.h>

#include <algorithm>

namespace decode {

template <std::size_t N>
static inline bmcl::StringView viewFromStaticString(const char(&data)[N])
{
    return bmcl::StringView(data, N-1);
}

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

bool Type::isGeneric() const
{
    return _typeKind == TypeKind::Generic;
}

bool Type::isGenericInstantiation() const
{
    return _typeKind == TypeKind::GenericInstantiation;
}

bool Type::isGenericParameter() const
{
    return _typeKind == TypeKind::GenericParameter;
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

bmcl::StringView Type::renderTypeKind(TypeKind kind)
{
    switch (kind) {
    case TypeKind::Builtin:
        return viewFromStaticString("Builtin");
    case TypeKind::Reference:
        return viewFromStaticString("Reference");
    case TypeKind::Array:
        return viewFromStaticString("Array");
    case TypeKind::DynArray:
        return viewFromStaticString("DynArray");
    case TypeKind::Function:
        return viewFromStaticString("Function");
    case TypeKind::Imported:
        return viewFromStaticString("Imported");
    case TypeKind::Alias:
        return viewFromStaticString("Alias");
    case TypeKind::GenericInstantiation:
        return viewFromStaticString("GenericInstantiation");
    case TypeKind::GenericParameter:
        return viewFromStaticString("GenericParameter");
    case TypeKind::Enum:
        return viewFromStaticString("Enum");
    case TypeKind::Struct:
        return viewFromStaticString("Struct");
    case TypeKind::Variant:
        return viewFromStaticString("Variant");
    case TypeKind::Generic:
        return viewFromStaticString("Generic");
    }
    assert(false);
    BMCL_UNREACHABLE();
}

bmcl::StringView Type::renderTypeKind() const
{
    return renderTypeKind(_typeKind);
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

const GenericType* Type::asGeneric() const
{
    assert(isGeneric());
    return static_cast<const GenericType*>(this);
}

const GenericInstantiationType* Type::asGenericInstantiation() const
{
    assert(isGenericInstantiation());
    return static_cast<const GenericInstantiationType*>(this);
}

const GenericParameterType* Type::asGenericParemeter() const
{
    assert(isGenericParameter());
    return static_cast<const GenericParameterType*>(this);
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

GenericType* Type::asGeneric()
{
    assert(isGeneric());
    return static_cast<GenericType*>(this);
}

GenericInstantiationType* Type::asGenericInstantiation()
{
    assert(isGenericInstantiation());
    return static_cast<GenericInstantiationType*>(this);
}

GenericParameterType* Type::asGenericParemeter()
{
    assert(isGenericParameter());
    return static_cast<GenericParameterType*>(this);
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

bmcl::Option<std::size_t> Type::fixedSize() const
{
    const Type* type = resolveFinalType();
    if (type->isArray()) {
        auto size = type->asArray()->elementType()->fixedSize();
        if (size.isSome()) {
            return size.unwrap() * type->asArray()->elementCount();
        }
        return bmcl::None;
    }
    if (!type->isBuiltin()) {
        return bmcl::None;
    }
    switch (type->asBuiltin()->builtinTypeKind()) {
        //TODO: calc usize/isize types depending on target
    case BuiltinTypeKind::USize:
    case BuiltinTypeKind::ISize:
    case BuiltinTypeKind::Varint:
    case BuiltinTypeKind::Varuint:
    case BuiltinTypeKind::Void:
        return bmcl::None;
    case BuiltinTypeKind::Bool:
    case BuiltinTypeKind::Char:
    case BuiltinTypeKind::U8:
    case BuiltinTypeKind::I8:
        return 1;
    case BuiltinTypeKind::U16:
    case BuiltinTypeKind::I16:
        return 2;
    case BuiltinTypeKind::F32:
    case BuiltinTypeKind::U32:
    case BuiltinTypeKind::I32:
        return 4;
    case BuiltinTypeKind::U64:
    case BuiltinTypeKind::I64:
    case BuiltinTypeKind::F64:
        return 8;
    }
    return bmcl::None;
}

static inline EncodedSizes varuintEncodedSizes()
{
    return {1, 8};
}

static inline EncodedSizes ptrEncodedSizes()
{
    return {4, 8};
}

static EncodedSizes variantFieldSize(const VariantField* field)
{
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant:
        return {0, 0};
    case VariantFieldKind::Tuple: {
        EncodedSizes sizes(0, 0);
        for (const Type* type : field->asTupleField()->typesRange()) {
            sizes += type->encodedSizes();
        }
        return sizes;
    }
    case VariantFieldKind::Struct: {
        EncodedSizes sizes(0, 0);
        for (const Field* f : field->asStructField()->fieldsRange()) {
            sizes += f->type()->encodedSizes();
        }
        return sizes;
    }
    }
    assert(false);
    return EncodedSizes(0, 0);
}

EncodedSizes Type::encodedSizes() const
{
    switch (typeKind()) {
    case TypeKind::Builtin:
        switch (asBuiltin()->builtinTypeKind()) {
        case BuiltinTypeKind::USize:
        case BuiltinTypeKind::ISize:
            return ptrEncodedSizes();
        case BuiltinTypeKind::Varint:
        case BuiltinTypeKind::Varuint:
            return varuintEncodedSizes();
        case BuiltinTypeKind::U8:
        case BuiltinTypeKind::I8:
        case BuiltinTypeKind::Bool:
        case BuiltinTypeKind::Char:
            return {1, 1};
        case BuiltinTypeKind::U16:
        case BuiltinTypeKind::I16:
            return {2, 2};
        case BuiltinTypeKind::F32:
        case BuiltinTypeKind::U32:
        case BuiltinTypeKind::I32:
            return {4, 4};
        case BuiltinTypeKind::U64:
        case BuiltinTypeKind::I64:
        case BuiltinTypeKind::F64:
            return {8, 8};
        case BuiltinTypeKind::Void:
            return {0, 0};
        }
        break;
    case TypeKind::Reference:
        return ptrEncodedSizes();
    case TypeKind::Array: {
        std::size_t n = asArray()->elementCount();
        return EncodedSizes{1, bmcl::varuintEncodedSize(n)} + asArray()->elementType()->encodedSizes() * n;
    }
    case TypeKind::DynArray: {
        std::size_t n = asDynArray()->maxSize();
        return EncodedSizes{1, bmcl::varuintEncodedSize(n)} + asDynArray()->elementType()->encodedSizes() * EncodedSizes(0, n);
    }
    case TypeKind::Function:
        return ptrEncodedSizes();
    case TypeKind::Enum: {
        std::uint64_t max = 0;
        for (const EnumConstant* c : asEnum()->constantsRange()) {
            max = std::max<std::uint64_t>(max, bmcl::zigZagEncode(c->value()));
        }
        return {1, bmcl::varuintEncodedSize(max)};
    }
    case TypeKind::Struct: {
        EncodedSizes sizes(0, 0);
        for (const Field* field : asStruct()->fieldsRange()) {
            sizes += field->type()->encodedSizes();
        }
        return sizes;
    }
    case TypeKind::Variant: {
        std::size_t numFields = asVariant()->fieldsRange().size();
        if (numFields == 0) {
            return {1, 1};
        }
        auto it = asVariant()->fieldsBegin();
        EncodedSizes sizes = variantFieldSize(*it);
        ++it;
        for (;it < asVariant()->fieldsEnd(); ++it) {
            sizes.merge(variantFieldSize(*it));
        }
        return EncodedSizes(1, bmcl::varintEncodedSize(numFields)) + sizes;
    }
    case TypeKind::Imported:
        return asImported()->link()->encodedSizes();
    case TypeKind::Alias:
        return asAlias()->alias()->encodedSizes();
    case TypeKind::GenericInstantiation:
        return asGenericInstantiation()->instantiatedType()->encodedSizes();
    case TypeKind::GenericParameter:
    case TypeKind::Generic:
        //FIXME
        assert(false);
        return EncodedSizes(0, 0);
    }
    assert(false);
    return EncodedSizes(0, 0);
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
            //FIXME: compare sizes
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
        case TypeKind::Generic: {
            const GenericType* l = first->asGeneric();
            const GenericType* r = second->asGeneric();
            return l->innerType()->equals(r->innerType());
        }
        case TypeKind::GenericInstantiation:
            return first->asGenericInstantiation()->instantiatedType()->equals(second->asGenericInstantiation()->instantiatedType());
        case TypeKind::GenericParameter:
            return first->asGenericParemeter()->name() == second->asGenericParemeter()->name();
    }

    return true;
}

TopLevelType::TopLevelType(TypeKind kind, const ModuleInfo* info)
    : Type(kind)
    , _moduleInfo(info)
{
}

TopLevelType::~TopLevelType()
{
}

const ModuleInfo* TopLevelType::moduleInfo() const
{
    return _moduleInfo.get();
}

bmcl::StringView TopLevelType::moduleName() const
{
    return _moduleInfo->moduleName();
}

void TopLevelType::setModuleInfo(const ModuleInfo* info)
{
    _moduleInfo.reset(info);
}

NamedType::NamedType(TypeKind kind, bmcl::StringView name, const ModuleInfo* info)
    : TopLevelType(kind, info)
    , _name(name)
{
}

NamedType::~NamedType()
{
}


bmcl::StringView NamedType::name() const
{
    return _name;
}

void NamedType::setName(bmcl::StringView name)
{
    _name = name;
}

GenericParameterType::GenericParameterType(bmcl::StringView name, const ModuleInfo* info)
    : NamedType(TypeKind::GenericParameter, name, info)
{
}

GenericParameterType::~GenericParameterType()
{
}

GenericInstantiationType::GenericInstantiationType(bmcl::StringView name, bmcl::ArrayView<Rc<Type>> substitutedTypes, NamedType* type)
    : TopLevelType(TypeKind::GenericInstantiation, type->moduleInfo())
    , _genericName(name)
    , _substitutedTypes(substitutedTypes.begin(), substitutedTypes.end())
    , _type(type)
{
}

GenericInstantiationType::~GenericInstantiationType()
{
}

bmcl::ArrayView<Rc<Type>> GenericInstantiationType::substitutedTypes()
{
    return _substitutedTypes;
}

RcVec<Type>::ConstRange GenericInstantiationType:: substitutedTypesRange() const
{
    return _substitutedTypes;
}

RcVec<Type>::Range GenericInstantiationType::substitutedTypesRange()
{
    return _substitutedTypes;
}

bmcl::StringView GenericInstantiationType::genericName() const
{
    return _genericName;
}

const GenericType* GenericInstantiationType::genericType() const
{
    return _genericType.get();
}

GenericType* GenericInstantiationType::genericType()
{
    return _genericType.get();
}

void GenericInstantiationType::setGenericType(GenericType* type)
{
    _genericType.reset(type);
}

const NamedType* GenericInstantiationType::instantiatedType() const
{
    return _type.get();
}

NamedType* GenericInstantiationType::instantiatedType()
{
    return _type.get();
}

void GenericInstantiationType::setInstantiatedType(NamedType* type)
{
    _type.reset(type);
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

bmcl::StringView BuiltinType::renderedTypeName(BuiltinTypeKind kind)
{
    switch (kind) {
    case BuiltinTypeKind::USize:
        return viewFromStaticString("usize");
    case BuiltinTypeKind::ISize:
        return viewFromStaticString("isize");
    case BuiltinTypeKind::Varuint:
        return viewFromStaticString("varuint");
    case BuiltinTypeKind::Varint:
        return viewFromStaticString("varint");
    case BuiltinTypeKind::U8:
        return viewFromStaticString("u8");
    case BuiltinTypeKind::I8:
        return viewFromStaticString("i8");
    case BuiltinTypeKind::U16:
        return viewFromStaticString("u16");
    case BuiltinTypeKind::I16:
        return viewFromStaticString("i16");
    case BuiltinTypeKind::U32:
        return viewFromStaticString("u32");
    case BuiltinTypeKind::I32:
        return viewFromStaticString("i32");
    case BuiltinTypeKind::U64:
        return viewFromStaticString("u64");
    case BuiltinTypeKind::I64:
        return viewFromStaticString("i64");
    case BuiltinTypeKind::F32:
        return viewFromStaticString("f32");
    case BuiltinTypeKind::F64:
        return viewFromStaticString("f64");
    case BuiltinTypeKind::Bool:
        return viewFromStaticString("bool");
    case BuiltinTypeKind::Void:
        return viewFromStaticString("void");
    case BuiltinTypeKind::Char:
        return viewFromStaticString("char");
    }
    return bmcl::StringView::empty();
}

BuiltinTypeKind BuiltinType::builtinTypeKind() const
{
    return _builtinTypeKind;
}

DynArrayType::DynArrayType(std::uintmax_t maxSize, Type* elementType)
    : Type(TypeKind::DynArray)
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

FunctionType::FunctionType()
    : Type(TypeKind::Function)
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

void FunctionType::setReturnValue(bmcl::OptionPtr<Type> type)
{
    _returnValue.reset(type.data());
}

void FunctionType::setSelfArgument(bmcl::Option<SelfArgument> arg)
{
    _self = arg;
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

bmcl::OptionPtr<Field> StructType::fieldWithName(bmcl::StringView name)
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

void EnumType::addConstant(EnumConstant* constant)
{
    _constantDecls.emplace_back(constant);
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

GenericType::GenericType(bmcl::StringView name, bmcl::ArrayView<Rc<GenericParameterType>> parameters, NamedType* genericType)
    : NamedType(TypeKind::Generic, name, genericType->moduleInfo())
    , _parameters(parameters.begin(), parameters.end())
    , _type(genericType)
{
}

GenericType::~GenericType()
{
}

NamedType* GenericType::innerType()
{
    return _type.get();
}

const NamedType* GenericType::innerType() const
{
    return _type.get();
}

bmcl::ArrayView<Rc<GenericParameterType>> GenericType::parameters()
{
    return _parameters;
}

RcVec<GenericParameterType>::ConstRange GenericType::parametersRange() const
{
    return _parameters;
}

bmcl::Result<Rc<NamedType>, std::string> GenericType::instantiate(const bmcl::ArrayView<Rc<Type>> types)
{
    if (_parameters.size() != types.size()) {
        return std::string("invalid number of parameters");
    }

    Rc<Type> cloned = cloneAndSubstitute(_type.get(), types);
    if (cloned.isNull()) {
        return std::string("failed to substitute generic parameters");
    }
    return Rc<NamedType>(static_cast<NamedType*>(cloned.get()));
}

Rc<Field> GenericType::cloneAndSubstitute(Field* field, bmcl::ArrayView<Rc<Type>> types)
{
    Rc<Type> clonedType = cloneAndSubstitute(field->type(), types);
    Rc<Field> clonedField = new Field(field->name(), clonedType.get());
    if (field->rangeAttribute().isSome()) {
        clonedField->setRangeAttribute(field->rangeAttribute().unwrap());
    }
    return clonedField;
}

Rc<VariantField> GenericType::cloneAndSubstitute(VariantField* varField, bmcl::ArrayView<Rc<Type>> types)
{
    switch (varField->variantFieldKind()) {
    case VariantFieldKind::Constant:
        return varField;
    case VariantFieldKind::Tuple: {
        TupleVariantField* f = static_cast<TupleVariantField*>(varField);
        Rc<TupleVariantField> newField = new TupleVariantField(f->id(), f->name());
        for (Type* type : f->typesRange()) {
            Rc<Type> cloned = cloneAndSubstitute(type, types);
            newField->addType(cloned.get());
        }
        return newField;
    }
    case VariantFieldKind::Struct: {
        StructVariantField* f = static_cast<StructVariantField*>(varField);
        Rc<StructVariantField> newField = new StructVariantField(f->id(), f->name());
        for (Field* field : f->fieldsRange()) {
            Rc<Field> cloned = cloneAndSubstitute(field, types);
            newField->addField(cloned.get());
        }
        return newField;
    }
    }
    bmcl::panic("unreachable"); //FIXME: add macro
}

Rc<Type> GenericType::cloneAndSubstitute(Type* type, bmcl::ArrayView<Rc<Type>> types)
{
    switch (type->typeKind()) {
        case TypeKind::Builtin:
            return type;
        case TypeKind::Reference: {
            ReferenceType* ref = type->asReference();
            Rc<Type> cloned = cloneAndSubstitute(ref->pointee(), types);
            return new ReferenceType(ref->referenceKind(), ref->isMutable(), cloned.get());
        }
        case TypeKind::Array: {
            ArrayType* array = type->asArray();
            Rc<Type> cloned = cloneAndSubstitute(array->elementType(), types);
            return new ArrayType(array->elementCount(), cloned.get());
        }
        case TypeKind::DynArray: {
            DynArrayType* dynArray = type->asDynArray();
            Rc<Type> cloned = cloneAndSubstitute(dynArray->elementType(), types);
            return new DynArrayType(dynArray->maxSize(), cloned.get());
        }
        case TypeKind::Function: {
            FunctionType* func = type->asFunction();
            Rc<FunctionType> newFunc = new FunctionType();
            if (func->hasReturnValue()) {
                Rc<Type> cloned = cloneAndSubstitute(func->returnValue().unwrap(), types);
                newFunc->setReturnValue(cloned.get());
            }
            newFunc->setSelfArgument(func->selfArgument());
            for (Field* field : func->argumentsRange()) {
                Rc<Field> cloned = cloneAndSubstitute(field, types);
                newFunc->addArgument(cloned.get());
            }
            return newFunc;
        }
        case TypeKind::Enum: {
            return type;
        }
        case TypeKind::Struct: {
            StructType* structType = type->asStruct();
            Rc<StructType> newStruct = new StructType(structType->name(), structType->moduleInfo());
            for (Field* field : structType->fieldsRange()) {
                Rc<Field> cloned = cloneAndSubstitute(field, types);
                newStruct->addField(cloned.get());
            }
            return newStruct;
        }
        case TypeKind::Variant: {
            VariantType* variant = type->asVariant();
            Rc<VariantType> newVariant = new VariantType(variant->name(), variant->moduleInfo());
            for (VariantField* field : variant->fieldsRange()) {
                Rc<VariantField> cloned = cloneAndSubstitute(field, types);
                newVariant->addField(cloned.get());
            }
            return newVariant;

        }
        case TypeKind::Imported: {
            return type;
        }
        case TypeKind::Alias: {
            AliasType* alias = type->asAlias();
            Rc<Type> cloned = cloneAndSubstitute(alias->alias(), types);
            return new AliasType(alias->name(), alias->moduleInfo(), cloned.get());
        }
        case TypeKind::Generic: {
            assert(false);
            return nullptr;
        }
        case TypeKind::GenericInstantiation: {
            GenericInstantiationType* generic = type->asGenericInstantiation();
            Rc<Type> cloned = cloneAndSubstitute(generic->instantiatedType(), types);
            return new GenericInstantiationType(generic->genericName(), generic->substitutedTypes(), static_cast<NamedType*>(cloned.get()));
        }
        case TypeKind::GenericParameter: {
            GenericParameterType* genericParam = type->asGenericParemeter();
            auto it = std::find_if(_parameters.begin(), _parameters.end(), [genericParam](const Rc<GenericParameterType>& type) {
                return type->name() == genericParam->name();
            });
            if (it == _parameters.end()) {
                return nullptr;
            }
            std::size_t index = it - _parameters.begin();
            return types[index];
        }
    }
    bmcl::panic("unreachable"); //FIXME: add macro
}
}
