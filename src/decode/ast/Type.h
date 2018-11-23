/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"
#include "decode/core/NamedRc.h"
#include "decode/parser/Containers.h"
#include "decode/ast/DocBlockMixin.h"

#include <bmcl/Fwd.h>
#include <bmcl/Option.h>
#include <bmcl/StringView.h>
#include <bmcl/StringViewHash.h>

#include <cstdint>
#include <map>

namespace decode {

enum class TypeKind {
    Builtin,
    Reference,
    Array,
    DynArray,
    Function,
    Enum,
    Struct,
    Variant,
    Imported,
    Alias,
    Generic,
    GenericInstantiation,
    GenericParameter,
};

enum class BuiltinTypeKind {
    USize,
    ISize,
    Varint,
    Varuint,
    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
    F32,
    F64,
    Bool,
    Void,
    Char,
};

enum class ReferenceKind {
    Pointer,
    Reference,
};

enum class SelfArgument {
    Reference,
    MutReference,
    Value,
};

class ArrayType;
class DynArrayType;
class StructType;
class FunctionType;
class BuiltinType;
class AliasType;
class ImportedType;
class VariantType;
class EnumType;
class ReferenceType;
class GenericType;
class GenericInstantiationType;
class GenericParameterType;
class Field;
class ModuleInfo;
struct EncodedSizes;

class Type : public RefCountable, public DocBlockMixin {
public:
    using Pointer = Rc<Type>;
    using ConstPointer = Rc<const Type>;

    ~Type();

    const ArrayType* asArray() const;
    const DynArrayType* asDynArray() const;
    const StructType* asStruct() const;
    const FunctionType* asFunction() const;
    const BuiltinType* asBuiltin() const;
    const AliasType* asAlias() const;
    const ImportedType* asImported() const;
    const VariantType* asVariant() const;
    const EnumType* asEnum() const;
    const ReferenceType* asReference() const;
    const GenericType* asGeneric() const;
    const GenericInstantiationType* asGenericInstantiation() const;
    const GenericParameterType* asGenericParemeter() const;

    ArrayType* asArray();
    DynArrayType* asDynArray();
    StructType* asStruct();
    FunctionType* asFunction();
    BuiltinType* asBuiltin();
    AliasType* asAlias();
    ImportedType* asImported();
    VariantType* asVariant();
    EnumType* asEnum();
    ReferenceType* asReference();
    GenericType* asGeneric();
    GenericInstantiationType* asGenericInstantiation();
    GenericParameterType* asGenericParemeter();

    static bmcl::StringView renderTypeKind(TypeKind kind);
    bmcl::StringView renderTypeKind() const;

    TypeKind typeKind() const;
    const Type* resolveFinalType() const;

    bmcl::Option<std::size_t> fixedSize() const;
    EncodedSizes encodedSizes() const;

    bool isArray() const;
    bool isDynArray() const;
    bool isStruct() const;
    bool isFunction() const;
    bool isBuiltin() const;
    bool isAlias() const;
    bool isImported() const;
    bool isVariant() const;
    bool isEnum() const;
    bool isReference() const;
    bool isGeneric() const;
    bool isGenericInstantiation() const;
    bool isGenericParameter() const;

    bool isBuiltinChar() const;

    bool equals(const Type* other) const;

protected:
    Type(TypeKind kind);

private:
    TypeKind _typeKind;
};

class TopLevelType : public Type {
public:
    using Pointer = Rc<TopLevelType>;
    using ConstPointer = Rc<const TopLevelType>;

    ~TopLevelType();

    const ModuleInfo* moduleInfo() const;
    bmcl::StringView moduleName() const;
    void setModuleInfo(const ModuleInfo* info);

protected:
    TopLevelType(TypeKind kind, const ModuleInfo* info);

private:
    Rc<const ModuleInfo> _moduleInfo;
};

class NamedType : public TopLevelType {
public:
    using Pointer = Rc<NamedType>;
    using ConstPointer = Rc<const NamedType>;

    ~NamedType();

    bmcl::StringView name() const;
    void setName(bmcl::StringView name);

protected:
    NamedType(TypeKind kind, bmcl::StringView name, const ModuleInfo* info);

private:
    bmcl::StringView _name;
};

class GenericParameterType : public NamedType {
public:
    using Pointer = Rc<GenericParameterType>;
    using ConstPointer = Rc<const GenericParameterType>;

    GenericParameterType(bmcl::StringView name, const ModuleInfo* info);
    ~GenericParameterType();
};

class GenericType: public NamedType {
public:
    GenericType(bmcl::StringView name, bmcl::ArrayView<Rc<GenericParameterType>> parameters, NamedType* genericType);
    ~GenericType();

    NamedType* innerType();
    const NamedType* innerType() const;
    bmcl::ArrayView<Rc<GenericParameterType>> parameters();

    RcVec<GenericParameterType>::ConstRange parametersRange() const;

    bmcl::Result<Rc<NamedType>, std::string> instantiate(bmcl::ArrayView<Rc<Type>> types);

private:
    Rc<Type> cloneAndSubstitute(Type* type, bmcl::ArrayView<Rc<Type>> types);
    Rc<VariantField> cloneAndSubstitute(VariantField* field, bmcl::ArrayView<Rc<Type>> types);
    Rc<Field> cloneAndSubstitute(Field* field, bmcl::ArrayView<Rc<Type>> types);

    RcVec<GenericParameterType> _parameters;
    Rc<NamedType> _type;
};

class GenericInstantiationType : public TopLevelType {
public:
    using Pointer = Rc<GenericInstantiationType>;
    using ConstPointer = Rc<const GenericInstantiationType>;

    GenericInstantiationType(bmcl::StringView name, bmcl::ArrayView<Rc<Type>> substitutedTypes, NamedType* type);
    ~GenericInstantiationType();

    bmcl::ArrayView<Rc<Type>> substitutedTypes();

    RcVec<Type>::ConstRange substitutedTypesRange() const;
    RcVec<Type>::Range substitutedTypesRange();

    const NamedType* instantiatedType() const;
    NamedType* instantiatedType();
    void setInstantiatedType(NamedType* type);

    bmcl::StringView genericName() const;
    const GenericType* genericType() const;
    GenericType* genericType();
    void setGenericType(GenericType* type);

private:
    bmcl::StringView _genericName;
    RcVec<Type> _substitutedTypes;
    Rc<NamedType> _type;
    Rc<GenericType> _genericType;
};

class AliasType : public NamedType {
public:
    using Pointer = Rc<AliasType>;
    using ConstPointer = Rc<const AliasType>;

    AliasType(bmcl::StringView name, const ModuleInfo* info, Type* alias);
    ~AliasType();

    const Type* alias() const;
    Type* alias();
    void setAlias(AliasType* type);

private:
    Rc<Type> _alias;
};

class ReferenceType : public Type {
public:
    using Pointer = Rc<ReferenceType>;
    using ConstPointer = Rc<const ReferenceType>;

    ReferenceType(ReferenceKind kind, bool isMutable, Type* pointee);
    ~ReferenceType();

    bool isMutable() const;
    ReferenceKind referenceKind() const;
    const Type* pointee() const;
    Type* pointee();

    void setPointee(Type* pointee);
    void setMutable(bool isMutable);
    void setReferenceKind(ReferenceKind kind);

private:
    Rc<Type> _pointee;
    ReferenceKind _referenceKind;
    bool _isMutable;
};

class BuiltinType : public Type {
public:
    using Pointer = Rc<BuiltinType>;
    using ConstPointer = Rc<const BuiltinType>;

    BuiltinType(BuiltinTypeKind kind);
    ~BuiltinType();

    static bmcl::StringView renderedTypeName(BuiltinTypeKind kind);

    BuiltinTypeKind builtinTypeKind() const;

private:
    BuiltinTypeKind _builtinTypeKind;
};

class DynArrayType : public Type {
public:
    using Pointer = Rc<DynArrayType>;
    using ConstPointer = Rc<const DynArrayType>;

    DynArrayType(std::uintmax_t maxSize, Type* elementType);
    ~DynArrayType();

    const Type* elementType() const;
    Type* elementType();
    std::uintmax_t maxSize() const;

private:
    std::uintmax_t _maxSize;
    Rc<Type> _elementType;
};

class ArrayType : public Type {
public:
    using Pointer = Rc<ArrayType>;
    using ConstPointer = Rc<const ArrayType>;

    ArrayType(std::uintmax_t elementCount, Type* elementType);
    ~ArrayType();

    std::uintmax_t elementCount() const;
    const Type* elementType() const;
    Type* elementType();

private:
    std::uintmax_t _elementCount;
    Rc<Type> _elementType;
};

class ImportedType : public NamedType {
public:
    using Pointer = Rc<ImportedType>;
    using ConstPointer = Rc<const ImportedType>;

    ImportedType(bmcl::StringView name, bmcl::StringView importPath, const ModuleInfo* info, NamedType* link = nullptr);
    ~ImportedType();

    const NamedType* link() const;
    NamedType* link();

    void setLink(NamedType* link);

private:
    bmcl::StringView _importPath;
    Rc<NamedType> _link;
};

class FunctionType : public Type {
public:
    using Pointer = Rc<FunctionType>;
    using ConstPointer = Rc<const FunctionType>;

    FunctionType();
    ~FunctionType();

    bmcl::OptionPtr<Type> returnValue();
    bmcl::OptionPtr<const Type> returnValue() const;
    bool hasReturnValue() const;
    bool hasArguments() const;
    FieldVec::Iterator argumentsBegin();
    FieldVec::Iterator argumentsEnd();
    FieldVec::Range argumentsRange();
    FieldVec::ConstIterator argumentsBegin() const;
    FieldVec::ConstIterator argumentsEnd() const;
    FieldVec::ConstRange argumentsRange() const;
    bmcl::Option<SelfArgument> selfArgument() const;

    void addArgument(Field* field);
    void setReturnValue(bmcl::OptionPtr<Type> type);
    void setSelfArgument(bmcl::Option<SelfArgument> arg);

private:
    bmcl::Option<SelfArgument> _self;
    FieldVec _arguments;
    Rc<Type> _returnValue;
};

class StructType : public NamedType {
public:
    using Pointer = Rc<StructType>;
    using ConstPointer = Rc<const StructType>;
    using Fields = FieldVec;

    StructType(bmcl::StringView name, const ModuleInfo* info);
    ~StructType();

    Fields::ConstIterator fieldsBegin() const;
    Fields::ConstIterator fieldsEnd() const;
    Fields::ConstRange fieldsRange() const;
    Fields::Iterator fieldsBegin();
    Fields::Iterator fieldsEnd();
    Fields::Range fieldsRange();

    void addField(Field* field);

    const Field* fieldAt(std::size_t index) const;

    bmcl::OptionPtr<const Field> fieldWithName(bmcl::StringView name) const;
    bmcl::OptionPtr<Field> fieldWithName(bmcl::StringView name);
    bmcl::Option<std::size_t> indexOfField(const Field* field) const;

private:
    Fields _fields;
    RcSecondUnorderedMap<bmcl::StringView, Field> _nameToFieldMap;
};

class EnumConstant : public NamedRc, public DocBlockMixin {
public:
    using Pointer = Rc<EnumConstant>;
    using ConstPointer = Rc<const EnumConstant>;

    EnumConstant(bmcl::StringView name, std::int64_t value, bool isUserSet);
    ~EnumConstant();

    std::int64_t value() const;
    bool isUserSet() const;

private:
    std::int64_t _value;
    bool _isUserSet;
};

class EnumType : public NamedType {
public:
    using Pointer = Rc<EnumType>;
    using ConstPointer = Rc<const EnumType>;
    using Constants = RcVec<EnumConstant>;

    EnumType(bmcl::StringView name, const ModuleInfo* info);
    ~EnumType();

    Constants::ConstIterator constantsBegin() const;
    Constants::ConstIterator constantsEnd() const;
    Constants::ConstRange constantsRange() const;

    void addConstant(EnumConstant* constant);

private:
    Constants _constantDecls;
};

class VariantField;

class VariantType : public NamedType {
public:
    using Pointer = Rc<VariantType>;
    using ConstPointer = Rc<const VariantType>;
    using Fields = VariantFieldVec;

    VariantType(bmcl::StringView name, const ModuleInfo* info);
    ~VariantType();

    Fields::ConstIterator fieldsBegin() const;
    Fields::ConstIterator fieldsEnd() const;
    Fields::ConstRange fieldsRange() const;
    Fields::Iterator fieldsBegin();
    Fields::Iterator fieldsEnd();
    Fields::Range fieldsRange();

    void addField(VariantField* field);

private:
    VariantFieldVec _fields;
};

template <typename T>
inline TypeKind deferTypeKind();

template <>
inline TypeKind deferTypeKind<BuiltinType>()
{
    return TypeKind::Builtin;
}

template <>
inline TypeKind deferTypeKind<ReferenceType>()
{
    return TypeKind::Reference;
}

template <>
inline TypeKind deferTypeKind<ArrayType>()
{
    return TypeKind::Array;
}

template <>
inline TypeKind deferTypeKind<DynArrayType>()
{
    return TypeKind::DynArray;
}

template <>
inline TypeKind deferTypeKind<FunctionType>()
{
    return TypeKind::Function;
}

template <>
inline TypeKind deferTypeKind<EnumType>()
{
    return TypeKind::Enum;
}

template <>
inline TypeKind deferTypeKind<StructType>()
{
    return TypeKind::Struct;
}

template <>
inline TypeKind deferTypeKind<VariantType>()
{
    return TypeKind::Variant;
}

template <>
inline TypeKind deferTypeKind<ImportedType>()
{
    return TypeKind::Imported;
}

template <>
inline TypeKind deferTypeKind<AliasType>()
{
    return TypeKind::Alias;
}

template <>
inline TypeKind deferTypeKind<GenericType>()
{
    return TypeKind::Generic;
}

template <>
inline TypeKind deferTypeKind<GenericInstantiationType>()
{
    return TypeKind::GenericInstantiation;
}
}
