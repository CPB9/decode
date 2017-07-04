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
#include "decode/core/Hash.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Field.h"
#include "decode/parser/Containers.h"
#include "decode/parser/DocBlock.h"

#include <bmcl/StringView.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/Logging.h>

#include <cstdint>
#include <map>
#include <unordered_map>

namespace decode {

enum class TypeKind {
    Builtin,
    Reference,
    Array,
    Slice,
    Function,
    Enum,
    Struct,
    Variant,
    Imported,
    Alias,
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
class SliceType;
class StructType;
class FunctionType;
class BuiltinType;
class AliasType;
class ImportedType;
class VariantType;
class EnumType;
class ReferenceType;

class Type : public RefCountable, public DocBlockMixin {
public:
    const ArrayType* asArray() const;
    const SliceType* asSlice() const;
    const StructType* asStruct() const;
    const FunctionType* asFunction() const;
    const BuiltinType* asBuiltin() const;
    const AliasType* asAlias() const;
    const ImportedType* asImported() const;
    const VariantType* asVariant() const;
    const EnumType* asEnum() const;
    const ReferenceType* asReference() const;

    ArrayType* asArray();
    SliceType* asSlice();
    StructType* asStruct();
    FunctionType* asFunction();
    BuiltinType* asBuiltin();
    AliasType* asAlias();
    ImportedType* asImported();
    VariantType* asVariant();
    EnumType* asEnum();
    ReferenceType* asReference();

    TypeKind typeKind() const;

    bool isArray() const;
    bool isSlice() const;
    bool isStruct() const;
    bool isFunction() const;
    bool isBuiltin() const;
    bool isAlias() const;
    bool isImported() const;
    bool isVariant() const;
    bool isEnum() const;
    bool isReference() const;

protected:
    Type(TypeKind kind)
        : _typeKind(kind)
    {
    }

private:
    TypeKind _typeKind;
};

class NamedType : public Type {
public:
    const ModuleInfo* moduleInfo() const
    {
        return _moduleInfo.get();
    }

    bmcl::StringView moduleName() const
    {
        return _moduleInfo->moduleName();
    }

    bmcl::StringView name() const
    {
        return _name;
    }

    void setName(bmcl::StringView name)
    {
        _name = name;
    }

    void setModuleInfo(const ModuleInfo* info)
    {
        _moduleInfo.reset(info);
    }

protected:
    NamedType(TypeKind kind) //TODO: remove
        : Type(kind)
    {
    }

    NamedType(TypeKind kind, bmcl::StringView name, const ModuleInfo* info)
        : Type(kind)
        , _name(name)
        , _moduleInfo(info)
    {
    }

private:
    bmcl::StringView _name;
    Rc<const ModuleInfo> _moduleInfo;
};

class AliasType : public NamedType {
public:
    AliasType(bmcl::StringView name, const ModuleInfo* info, Type* alias)
        : NamedType(TypeKind::Alias, name, info)
        , _alias(alias)
    {
    }

    const Type* alias() const
    {
        return _alias.get();
    }

    Type* alias()
    {
        return _alias.get();
    }

    void setAlias(AliasType* type)
    {
        _alias.reset(type);
    }

private:
    Rc<Type> _alias;
};

class ReferenceType : public Type {
public:
    ReferenceType(ReferenceKind kind, bool isMutable, Type* pointee)
        : Type(TypeKind::Reference)
        , _pointee(pointee)
        , _referenceKind(kind)
        , _isMutable(isMutable)
    {
    }

    bool isMutable() const
    {
        return _isMutable;
    }

    ReferenceKind referenceKind() const
    {
        return _referenceKind;
    }

    const Type* pointee() const
    {
        return _pointee.get();
    }

    Type* pointee()
    {
        return _pointee.get();
    }

    void setPointee(Type* pointee)
    {
        _pointee.reset(pointee);
    }

    void setMutable(bool isMutable)
    {
        _isMutable = isMutable;
    }

    void setReferenceKind(ReferenceKind kind)
    {
        _referenceKind = kind;
    }

private:
    Rc<Type> _pointee;
    ReferenceKind _referenceKind;
    bool _isMutable;
};

class BuiltinType : public Type {
public:
    BuiltinType(BuiltinTypeKind kind)
        : Type(TypeKind::Builtin)
        , _builtinTypeKind(kind)
    {
    }

    BuiltinTypeKind builtinTypeKind() const
    {
        return _builtinTypeKind;
    }

private:
    BuiltinTypeKind _builtinTypeKind;
};

class SliceType : public Type {
public:
    SliceType(const ModuleInfo* info, Type* elementType)
        : Type(TypeKind::Slice)
        , _moduleInfo(info)
        , _elementType(elementType)
    {
    }

    const Type* elementType() const
    {
        return _elementType.get();
    }

    Type* elementType()
    {
        return _elementType.get();
    }

    const ModuleInfo* moduleInfo() const
    {
        return _moduleInfo.get();
    }

    bmcl::StringView moduleName() const
    {
        return _moduleInfo->moduleName();
    }

private:
    Rc<const ModuleInfo> _moduleInfo;
    Rc<Type> _elementType;
};

class ArrayType : public Type {
public:
    ArrayType(std::uintmax_t elementCount, Type* elementType)
        : Type(TypeKind::Array)
        , _elementCount(elementCount)
        , _elementType(elementType)
    {
    }

    std::uintmax_t elementCount() const
    {
        return _elementCount;
    }

    const Type* elementType() const
    {
        return _elementType.get();
    }

    Type* elementType()
    {
        return _elementType.get();
    }

private:
    std::uintmax_t _elementCount;
    Rc<Type> _elementType;
};

class ImportedType : public NamedType {
public:
    ImportedType(bmcl::StringView name, bmcl::StringView importPath, const ModuleInfo* info, NamedType* link = nullptr)
        : NamedType(TypeKind::Imported, name, info)
        , _importPath(importPath)
        , _link(link)
    {
    }

    const NamedType* link() const
    {
        return _link.get();
    }

    NamedType* link()
    {
        return _link.get();
    }

    void setLink(NamedType* link)
    {
        _link.reset(link);
    }

private:
    bmcl::StringView _importPath;
    Rc<NamedType> _link;
};

class FunctionType : public Type { //TODO: inherit from type
public:
    FunctionType(const ModuleInfo* info)
        : Type(TypeKind::Function)
        , _modInfo(info)
    {
    }

    bmcl::OptionPtr<Type> returnValue()
    {
        return _returnValue.get();
    }

    bmcl::OptionPtr<const Type> returnValue() const
    {
        return _returnValue.get();
    }

    bool hasReturnValue() const
    {
        return _returnValue.get() != nullptr;
    }

    bool hasArguments() const
    {
        return !_arguments.empty();
    }

    FieldVec::Iterator argumentsBegin()
    {
        return _arguments.begin();
    }

    FieldVec::Iterator argumentsEnd()
    {
        return _arguments.end();
    }

    FieldVec::Range argumentsRange()
    {
        return _arguments;
    }

    FieldVec::ConstIterator argumentsBegin() const
    {
        return _arguments.cbegin();
    }

    FieldVec::ConstIterator argumentsEnd() const
    {
        return _arguments.cend();
    }

    FieldVec::ConstRange argumentsRange() const
    {
        return _arguments;
    }

    bmcl::Option<SelfArgument> selfArgument() const
    {
        return _self;
    }

    void addArgument(Field* field)
    {
        _arguments.emplace_back(field);
    }

    void setReturnValue(Type* type)
    {
        _returnValue.reset(type);
    }

    void setSelfArgument(SelfArgument arg)
    {
        _self.emplace(arg);
    }

private:
    bmcl::Option<SelfArgument> _self;
    FieldVec _arguments;
    Rc<Type> _returnValue;
    Rc<const ModuleInfo> _modInfo;
};

class StructType : public NamedType {
public:
    using Fields = FieldVec;

    StructType(bmcl::StringView name, const ModuleInfo* info)
        : NamedType(TypeKind::Struct, name, info)
    {
    }

    Fields::ConstIterator fieldsBegin() const
    {
        return _fields.cbegin();
    }

    Fields::ConstIterator fieldsEnd() const
    {
        return _fields.cend();
    }

    Fields::ConstRange fieldsRange() const
    {
        return _fields;
    }

    Fields::Iterator fieldsBegin()
    {
        return _fields.begin();
    }

    Fields::Iterator fieldsEnd()
    {
        return _fields.end();
    }

    Fields::Range fieldsRange()
    {
        return _fields;
    }

    void addField(Field* field)
    {
        _fields.emplace_back(field);
        _nameToFieldMap.emplace(field->name(), field);
    }

    bmcl::OptionPtr<const Field> fieldWithName(bmcl::StringView name) const
    {
        return _nameToFieldMap.findValueWithKey(name);
    }

    bmcl::Option<std::size_t> indexOfField(const Field* field) const
    {
        auto it = std::find(_fields.begin(), _fields.end(), field);
        if (it == _fields.end()) {
            return bmcl::None;
        }
        return std::distance(_fields.begin(), it);
    }

private:
    Fields _fields;
    RcSecondUnorderedMap<bmcl::StringView, Field> _nameToFieldMap;
};

class EnumConstant : public NamedRc, public DocBlockMixin {
public:
    EnumConstant(bmcl::StringView name, std::int64_t value, bool isUserSet)
        : NamedRc(name)
        , _value(value)
        , _isUserSet(isUserSet)
    {
    }

    std::int64_t value() const
    {
        return _value;
    }

    bool isUserSet() const
    {
        return _isUserSet;
    }

private:
    std::int64_t _value;
    bool _isUserSet;
};

class EnumType : public NamedType {
public:
    using Constants = RcSecondUnorderedMap<std::int64_t, EnumConstant>;

    EnumType(bmcl::StringView name, const ModuleInfo* info)
        : NamedType(TypeKind::Enum, name, info)
    {
    }

    Constants::ConstIterator constantsBegin() const
    {
        return _constantDecls.cbegin();
    }

    Constants::ConstIterator constantsEnd() const
    {
        return _constantDecls.cend();
    }

    Constants::ConstRange constantsRange() const
    {
        return _constantDecls;
    }

    bool addConstant(EnumConstant* constant)
    {
        auto pair = _constantDecls.emplace(constant->value(), constant);
        return pair.second;
    }

private:
    Constants _constantDecls;
};

class VariantField;

class VariantType : public NamedType {
public:
    using Fields = VariantFieldVec;

    VariantType(bmcl::StringView name, const ModuleInfo* info)
        : NamedType(TypeKind::Variant, name, info)
    {
    }

    Fields::ConstIterator fieldsBegin() const
    {
        return _fields.cbegin();
    }

    Fields::ConstIterator fieldsEnd() const
    {
        return _fields.cend();
    }

    Fields::ConstRange fieldsRange() const
    {
        return _fields;
    }

    Fields::Iterator fieldsBegin()
    {
        return _fields.begin();
    }

    Fields::Iterator fieldsEnd()
    {
        return _fields.end();
    }

    Fields::Range fieldsRange()
    {
        return _fields;
    }

    void addField(VariantField* field)
    {
        _fields.emplace_back(field);
    }

private:
    VariantFieldVec _fields;
};

inline bool Type::isArray() const
{
    return _typeKind == TypeKind::Array;
}

inline bool Type::isSlice() const
{
    return _typeKind == TypeKind::Slice;
}

inline bool Type::isStruct() const
{
    return _typeKind == TypeKind::Struct;
}

inline bool Type::isFunction() const
{
    return _typeKind == TypeKind::Function;
}

inline bool Type::isBuiltin() const
{
    return _typeKind == TypeKind::Builtin;
}

inline bool Type::isAlias() const
{
    return _typeKind == TypeKind::Alias;
}

inline bool Type::isImported() const
{
    return _typeKind == TypeKind::Imported;
}

inline bool Type::isVariant() const
{
    return _typeKind == TypeKind::Variant;
}

inline bool Type::isEnum() const
{
    return _typeKind == TypeKind::Enum;
}

inline bool Type::isReference() const
{
    return _typeKind == TypeKind::Reference;
}

inline TypeKind Type::typeKind() const
{
    return _typeKind;
}

inline const ArrayType* Type::asArray() const
{
    assert(isArray());
    return static_cast<const ArrayType*>(this);
}

inline const SliceType* Type::asSlice() const
{
    assert(isSlice());
    return static_cast<const SliceType*>(this);
}

inline const StructType* Type::asStruct() const
{
    assert(isStruct());
    return static_cast<const StructType*>(this);
}

inline const FunctionType* Type::asFunction() const
{
    assert(isFunction());
    return static_cast<const FunctionType*>(this);
}

inline const BuiltinType* Type::asBuiltin() const
{
    assert(isBuiltin());
    return static_cast<const BuiltinType*>(this);
}

inline const AliasType* Type::asAlias() const
{
    assert(isAlias());
    return static_cast<const AliasType*>(this);
}

inline const ImportedType* Type::asImported() const
{
    assert(isImported());
    return static_cast<const ImportedType*>(this);
}

inline const VariantType* Type::asVariant() const
{
    assert(isVariant());
    return static_cast<const VariantType*>(this);
}

inline const EnumType* Type::asEnum() const
{
    assert(isEnum());
    return static_cast<const EnumType*>(this);
}

inline const ReferenceType* Type::asReference() const
{
    assert(isReference());
    return static_cast<const ReferenceType*>(this);
}

inline ArrayType* Type::asArray()
{
    assert(isArray());
    return static_cast<ArrayType*>(this);
}

inline SliceType* Type::asSlice()
{
    assert(isSlice());
    return static_cast<SliceType*>(this);
}

inline StructType* Type::asStruct()
{
    assert(isStruct());
    return static_cast<StructType*>(this);
}

inline FunctionType* Type::asFunction()
{
    assert(isFunction());
    return static_cast<FunctionType*>(this);
}

inline BuiltinType* Type::asBuiltin()
{
    assert(isBuiltin());
    return static_cast<BuiltinType*>(this);
}

inline AliasType* Type::asAlias()
{
    assert(isAlias());
    return static_cast<AliasType*>(this);
}

inline ImportedType* Type::asImported()
{
    assert(isImported());
    return static_cast<ImportedType*>(this);
}

inline VariantType* Type::asVariant()
{
    assert(isVariant());
    return static_cast<VariantType*>(this);
}

inline EnumType* Type::asEnum()
{
    assert(isEnum());
    return static_cast<EnumType*>(this);
}

inline ReferenceType* Type::asReference()
{
    assert(isReference());
    return static_cast<ReferenceType*>(this);
}
}
