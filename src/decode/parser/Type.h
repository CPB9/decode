#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Field.h"

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

class Type : public RefCountable {
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

    bool isArray() const
    {
        return _typeKind == TypeKind::Array;
    }

    bool isSlice() const
    {
        return _typeKind == TypeKind::Slice;
    }

    bool isStruct() const
    {
        return _typeKind == TypeKind::Struct;
    }

    bool isFunction() const
    {
        return _typeKind == TypeKind::Function;
    }

    bool isBuiltin() const
    {
        return _typeKind == TypeKind::Builtin;
    }

    bool isAlias() const
    {
        return _typeKind == TypeKind::Alias;
    }

    bool isImported() const
    {
        return _typeKind == TypeKind::Imported;
    }

    bool isVariant() const
    {
        return _typeKind == TypeKind::Variant;
    }

    bool isEnum() const
    {
        return _typeKind == TypeKind::Enum;
    }

    bool isReference() const
    {
        return _typeKind == TypeKind::Reference;
    }

    TypeKind typeKind() const
    {
        return _typeKind;
    }

protected:
    Type(TypeKind kind)
        : _typeKind(kind)
    {
    }

private:
    friend class Parser;
    friend class Package;
    friend class Value;


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

    TypeKind _typeKind;
};

class NamedType : public Type {
public:
    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

    bmcl::StringView moduleName() const
    {
        return _moduleInfo->moduleName();
    }

    bmcl::StringView name() const
    {
        return _name;
    }

protected:
    NamedType(TypeKind kind)
        : Type(kind)
    {
    }

private:
    friend class Parser;
    friend class Package;

    bmcl::StringView _name;
    Rc<ModuleInfo> _moduleInfo;
};

class AliasType : public NamedType {
public:
    const Rc<Type>& alias() const
    {
        return _alias;
    }

protected:
    AliasType()
        : NamedType(TypeKind::Alias)
    {
    }

private:
    friend class Parser;

    Rc<Type> _alias;
};

class ReferenceType : public Type {
public:
    ReferenceType(const Rc<Type>& pointee, ReferenceKind kind, bool isMutable = false)
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

    const Rc<Type>& pointee() const
    {
        return _pointee;
    }

protected:
    ReferenceType()
        : Type(TypeKind::Reference)
    {
    }

private:
    friend class Parser;

    Rc<Type> _pointee;
    ReferenceKind _referenceKind;
    bool _isMutable;
};

class BuiltinType : public Type {
public:

    BuiltinTypeKind builtinTypeKind() const
    {
        return _builtinTypeKind;
    }

protected:
    BuiltinType(BuiltinTypeKind kind)
        : Type(TypeKind::Builtin)
        , _builtinTypeKind(kind)
    {
    }

private:
    friend class Parser;

    BuiltinTypeKind _builtinTypeKind;
};

class SliceType : public Type {
public:

    const Rc<Type>& elementType() const
    {
        return _elementType;
    }

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

    bmcl::StringView moduleName() const
    {
        return _moduleInfo->moduleName();
    }


protected:
    SliceType()
        : Type(TypeKind::Slice)
    {
    }

private:
    friend class Parser;

    Rc<ModuleInfo> _moduleInfo;
    Rc<Type> _elementType;
};

class ArrayType : public Type {
public:

    std::uintmax_t elementCount() const
    {
        return _elementCount;
    }

    const Rc<Type>& elementType() const
    {
        return _elementType;
    }

protected:
    ArrayType()
        : Type(TypeKind::Array)
    {
    }

private:
    friend class Parser;

    std::uintmax_t _elementCount;
    Rc<Type> _elementType;
};

class ImportedType : public NamedType {
public:
    const Rc<NamedType>& link() const
    {
        return _link;
    }

protected:
    ImportedType()
        : NamedType(TypeKind::Imported)
    {
    }

private:
    friend class Parser;
    friend class Package;

    bmcl::StringView _importPath;
    Rc<NamedType> _link;
};

class FunctionType : public NamedType {
public:

    const bmcl::Option<Rc<Type>>& returnValue() const
    {
        return _returnValue;
    }

    const std::vector<Rc<Field>>& arguments() const
    {
        return _arguments;
    }

    bmcl::Option<SelfArgument> selfArgument() const
    {
        return _self;
    }

protected:
    FunctionType()
        : NamedType(TypeKind::Function)
    {
    }

private:
    friend class Parser;

    bmcl::Option<SelfArgument> _self;
    std::vector<Rc<Field>> _arguments;
    bmcl::Option<Rc<Type>> _returnValue;
};

class StructType : public NamedType {
public:
    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    StructType()
        : NamedType(TypeKind::Struct)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class EnumConstant : public NamedRc {
public:

    std::int64_t value() const
    {
        return _value;
    }

    bool isUserSet() const
    {
        return _isUserSet;
    }

protected:
    EnumConstant() = default;

private:
    friend class Parser;

    std::int64_t _value;
    bool _isUserSet;
};

class EnumType : public NamedType {
public:

    const std::map<std::int64_t, Rc<EnumConstant>>& constants() const
    {
        return _constantDecls;
    }

protected:

    bool addConstant(const Rc<EnumConstant>& constant)
    {
        auto pair = _constantDecls.emplace(constant->value(), constant);
        return pair.second;
    }

    EnumType()
        : NamedType(TypeKind::Enum)
    {
    }

private:
    friend class Parser;

    std::map<std::int64_t, Rc<EnumConstant>> _constantDecls;
};

class VariantField;

class VariantType : public NamedType {
public:
    const std::vector<Rc<VariantField>>& fields() const
    {
        return _fields;
    }

protected:
    VariantType()
        : NamedType(TypeKind::Variant)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<VariantField>> _fields;
};

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
