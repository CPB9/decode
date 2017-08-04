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
#include "decode/core/NamedRc.h"
#include "decode/parser/Containers.h"
#include "decode/ast/DocBlockMixin.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

#include <cstdint>
#include <map>

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
class Field;
class ModuleInfo;

class Type : public RefCountable, public DocBlockMixin {
public:
    ~Type();

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
    Type(TypeKind kind);

private:
    TypeKind _typeKind;
};

class NamedType : public Type {
public:
    ~NamedType();

    const ModuleInfo* moduleInfo() const;
    bmcl::StringView moduleName() const;
    bmcl::StringView name() const;
    void setName(bmcl::StringView name);
    void setModuleInfo(const ModuleInfo* info);

protected:
    NamedType(TypeKind kind, bmcl::StringView name, const ModuleInfo* info);

private:
    bmcl::StringView _name;
    Rc<const ModuleInfo> _moduleInfo;
};

class AliasType : public NamedType {
public:
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
    BuiltinType(BuiltinTypeKind kind);
    ~BuiltinType();

    BuiltinTypeKind builtinTypeKind() const;

private:
    BuiltinTypeKind _builtinTypeKind;
};

class SliceType : public Type {
public:
    SliceType(const ModuleInfo* info, Type* elementType);
    ~SliceType();

    const Type* elementType() const;
    Type* elementType();
    const ModuleInfo* moduleInfo() const;
    bmcl::StringView moduleName() const;

private:
    Rc<const ModuleInfo> _moduleInfo;
    Rc<Type> _elementType;
};

class ArrayType : public Type {
public:
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
    FunctionType(const ModuleInfo* info);
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
    void setReturnValue(Type* type);
    void setSelfArgument(SelfArgument arg);

private:
    bmcl::Option<SelfArgument> _self;
    FieldVec _arguments;
    Rc<Type> _returnValue;
    Rc<const ModuleInfo> _modInfo;
};

class StructType : public NamedType {
public:
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

    bmcl::OptionPtr<const Field> fieldWithName(bmcl::StringView name) const;
    bmcl::Option<std::size_t> indexOfField(const Field* field) const;

private:
    Fields _fields;
    RcSecondUnorderedMap<bmcl::StringView, Field> _nameToFieldMap;
};

class EnumConstant : public NamedRc, public DocBlockMixin {
public:
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
    using Constants = RcSecondMap<std::int64_t, EnumConstant>;

    EnumType(bmcl::StringView name, const ModuleInfo* info);
    ~EnumType();

    Constants::ConstIterator constantsBegin() const;
    Constants::ConstIterator constantsEnd() const;
    Constants::ConstRange constantsRange() const;

    bool addConstant(EnumConstant* constant);

private:
    Constants _constantDecls;
};

class VariantField;

class VariantType : public NamedType {
public:
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
}
