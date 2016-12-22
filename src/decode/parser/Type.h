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
};

enum class BuiltinTypeKind {
    Unknown,
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

class Type : public RefCountable {
public:

    const ArrayType* asArray() const;
    const SliceType* asSlice() const;
    const StructType* asStruct() const;

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

    TypeKind typeKind() const
    {
        return _typeKind;
    }

    bmcl::StringView name() const
    {
        return _name;
    }

    bmcl::StringView moduleName() const
    {
        return _moduleInfo->moduleName();
    }

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

protected:
    Type(TypeKind kind)
        : _typeKind(kind)
    {
    }

private:
    friend class Parser;
    friend class Package;

    ArrayType* asArray();
    SliceType* asSlice();
    StructType* asStruct();

    bmcl::StringView _name;
    TypeKind _typeKind;
    Rc<ModuleInfo> _moduleInfo;
};

class NamedType : public Type {
protected:
    NamedType(TypeKind kind)
        : Type(kind)
    {
    }
};

class ReferenceType : public Type {
public:
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
    BuiltinType()
        : Type(TypeKind::Builtin)
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

protected:
    SliceType()
        : Type(TypeKind::Slice)
    {
    }

private:
    friend class Parser;

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
    const Rc<Type>& link() const
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
    Rc<Type> _link;
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
}
