#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Location.h"
#include "decode/core/Hash.h"
#include "decode/parser/Span.h"
#include "decode/parser/ModuleInfo.h"

#include <bmcl/Option.h>
#include <bmcl/Either.h>

#include <vector>
#include <set>
#include <unordered_map>
#include <cassert>
#include <map>

namespace decode {

class ImportedType;

enum class TypeKind {
    Reference,
    Slice,
    Builtin,
    Array,
    Struct,
    Enum,
    Variant,
    Component,
    Unresolved,
    Function,
    FnPointer,
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

class Decl : public RefCountable {
public:

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

protected:
    Decl() = default;

    void cloneDeclTo(Decl* dest)
    {
        dest->_startLoc = _startLoc;
        dest->_endLoc = _endLoc;
        dest->_start = _start;
        dest->_end = _end;
        dest->_moduleInfo = _moduleInfo;
    }

private:
    friend class Parser;

    Location _startLoc;
    Location _endLoc;
    const char* _start;
    const char* _end;
    Rc<ModuleInfo> _moduleInfo;
};

class NamedDecl : public Decl {
public:
    bmcl::StringView name() const
    {
        return _name;
    }

protected:
    NamedDecl() = default;

private:
    friend class Parser;

    bmcl::StringView _name;
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
        return _moduleName;
    }


protected:
    Type(TypeKind kind)
        : _typeKind(kind)
    {
    }

private:
    friend class Parser;

    ArrayType* asArray();
    SliceType* asSlice();
    StructType* asStruct();

    bmcl::StringView _name;
    TypeKind _typeKind;
    bmcl::StringView _moduleName;
};

class TypeDecl : public Decl {
public:

    const Rc<Type>& type() const
    {
        return _type;
    }

protected:
    TypeDecl() = default;

private:
    friend class Parser;

    Rc<Type> _type;
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

class Module : public NamedDecl {
public:

protected:
    Module() = default;

private:
    friend class Parser;
};

class ImportedType;

class Import : public Decl {
public:
    bmcl::StringView path() const
    {
        return _importPath;
    }

protected:
    Import() = default;

private:
    friend class Parser;
    bmcl::StringView _importPath;
    std::vector<bmcl::StringView> _types;
};

class UnresolvedType : public Type {
public:

protected:
    UnresolvedType()
        : Type(TypeKind::Unresolved)
    {
    }

private:
    friend class Parser;

    bmcl::StringView _importPath;
};

class FnPointer : public Type {
public:
    const bmcl::Option<Rc<Type>>& returnValue() const
    {
        return _returnValue;
    }

    const std::vector<Rc<Type>>& arguments() const
    {
        return _arguments;
    }

protected:
    FnPointer()
        : Type(TypeKind::FnPointer)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<Type>> _arguments;
    bmcl::Option<Rc<Type>> _returnValue;
};

class Field : public NamedDecl {
public:

    const Rc<Type>& type() const
    {
        return _type;
    }

protected:
    Field() = default;

private:
    friend class Parser;

    Rc<Type> _type;
};

class Tag : public Type {
public:

protected:
    Tag(TypeKind kind)
        : Type(kind)
    {
    }

private:
    friend class Parser;

};

enum SelfArgument {
    Reference,
    MutReference,
    Value,
};

class Function : public Type {
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
    Function()
        : Type(TypeKind::Function)
    {
    }

private:
    friend class Parser;

    bmcl::Option<SelfArgument> _self;
    std::vector<Rc<Field>> _arguments;
    bmcl::Option<Rc<Type>> _returnValue;
};

class ImplBlock : public NamedDecl {
public:

    const std::vector<Rc<Function>>& functions() const
    {
        return _funcs;
    }

protected:
    ImplBlock() = default;

private:
    friend class Parser;

    std::vector<Rc<Function>> _funcs;
};

template<typename T>
class RefCountableVector : public std::vector<T>, public RefCountable {
};

class FieldList : public RefCountableVector<Rc<Field>> {
public:
    bmcl::Option<const Rc<Field>&> fieldWithName(bmcl::StringView name)
    {
        for (const Rc<Field>& value : *this) {
            if (value->name() == name) {
                return value;
            }
        }
        return bmcl::None;
    }
};

class Record : public Tag {
public:
    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    Record(TypeKind kind)
        : Tag(kind)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class StructType : public Record {
public:

protected:
    StructType()
        : Record(TypeKind::Struct)
    {
    }

private:
    friend class Parser;
};

class EnumConstant : public NamedDecl {
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

class Enum : public Tag {
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

    Enum()
        : Tag(TypeKind::Enum)
    {
    }

private:
    friend class Parser;

    std::map<std::int64_t, Rc<EnumConstant>> _constantDecls;
};

class VariantField;

class Variant : public Tag {
public:
    const std::vector<Rc<VariantField>>& fields() const
    {
        return _fields;
    }

protected:
    Variant()
        : Tag(TypeKind::Variant)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<VariantField>> _fields;
};

enum VariantFieldKind {
    Constant,
    Tuple,
    Struct
};

class VariantField : public NamedDecl {
public:

    VariantFieldKind variantFieldKind() const
    {
        return _variantFieldKind;
    }

protected:
    VariantField(VariantFieldKind kind)
        : _variantFieldKind(kind)
    {
    }

    VariantFieldKind _variantFieldKind;

private:
    friend class Parser;

};

class ConstantVariantField : public VariantField {
protected:
    ConstantVariantField()
        : VariantField(VariantFieldKind::Constant)
    {
    }

private:
    friend class Parser;
};

class StructVariantField : public VariantField {
public:
    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    StructVariantField()
        : VariantField(VariantFieldKind::Struct)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class TupleVariantField : public VariantField {
public:

    const std::vector<Rc<Type>>& types() const
    {
        return _types;
    }

protected:
    TupleVariantField()
        : VariantField(VariantFieldKind::Tuple)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<Type>> _types;
};

class Parameters: public Decl {
public:

    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    Parameters()
        : _fields(new FieldList)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class Commands: public Decl {
public:

    const std::vector<Rc<Function>>& functions() const
    {
        return _functions;
    }

protected:
    Commands() = default;

private:
    friend class Parser;

    std::vector<Rc<Function>> _functions;
};

enum class AccessorKind {
    Field,
    Subscript,
};

class Accessor : public RefCountable {
public:

protected:
    Accessor(AccessorKind kind)
        : _accessorKind(kind)
    {
    }

private:
    friend class Parser;

    AccessorKind _accessorKind;
};

class Range {
private:
    friend class Parser;

    bmcl::Option<uintmax_t> _lowerBound;
    bmcl::Option<uintmax_t> _upperBound;
};

class FieldAccessor : public Accessor {
public:

protected:
    FieldAccessor()
        : Accessor(AccessorKind::Field)
    {
    }

private:
    friend class Parser;

    Rc<Field> _field;
};

class SubscriptAccessor : public Accessor {
public:

protected:
    SubscriptAccessor()
        : Accessor(AccessorKind::Subscript)
        , _subscript(bmcl::InPlaceSecond)
    {
    }

private:
    friend class Parser;

    Rc<Type> _type;
    bmcl::Either<Range, uintmax_t> _subscript;
};

class StatusRegexp : public RefCountable {
public:
protected:
    StatusRegexp() = default;

private:
    friend class Parser;

    std::vector<Rc<Accessor>> _accessors;
};

class Statuses: public Decl {
public:

protected:
    Statuses() = default;

private:
    friend class Parser;

    std::unordered_map<std::size_t, std::vector<Rc<StatusRegexp>>> _regexps;
};

class Component : public Tag {
public:

    const Rc<Parameters>& parameters() const
    {
        return _params;
    }

    const Rc<Commands>& commands() const
    {
        return _cmds;
    }

protected:
    Component()
        : Tag(TypeKind::Component)
    {
    }

private:
    friend class Parser;

    Rc<Parameters> _params;
    Rc<Commands> _cmds;
    Rc<Statuses> _statuses;
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
