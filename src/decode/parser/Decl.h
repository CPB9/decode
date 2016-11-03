#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Location.h"
#include "decode/core/Hash.h"
#include "decode/parser/Span.h"
#include "decode/parser/ModuleInfo.h"

#include <vector>
#include <set>
#include <unordered_map>

namespace decode {

class ImportedType;

enum class TypeKind {
    Reference,
    Builtin,
    Array,
    Struct,
    Enum,
    Variant,
    Component,
    Resolved,
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
};

enum class ReferenceKind {
    Pointer,
    Reference,
    Slice
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

class Type : public NamedDecl {
public:

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

    TypeKind _typeKind;
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
    std::vector<Rc<ImportedType>> _types;
};

class ImportedType : public Type {
public:
    const Rc<Type>& importedType() const
    {
        return _importedType;
    }

protected:
    ImportedType()
        : Type(TypeKind::Imported)
    {
    }

private:
    friend class Parser;
    bmcl::StringView _importPath;
    Rc<Type> _importedType;
};

class ResolvedType : public Type {
public:
    const Rc<Type>& resolvedType() const
    {
        return _resolvedType;
    }

protected:
    ResolvedType()
        : Type(TypeKind::Resolved)
    {
    }

private:
    friend class Parser;

    Rc<Type> _resolvedType;
};

class Field : public NamedDecl {
public:

    const Rc<Type> type() const
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

class FieldList : public Decl {
public:
    const std::vector<Rc<Field>> fields() const
    {
        return _fields;
    }

protected:
    FieldList() = default;

private:
    friend class Parser;

    std::vector<Rc<Field>> _fields;
    std::unordered_map<bmcl::StringView, Rc<Field>> _fieldNameToFieldMap;
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

class StructDecl : public Record {
public:

protected:
    StructDecl()
        : Record(TypeKind::Struct)
    {
    }

private:
    friend class Parser;
};

class EnumConstant : public NamedDecl {
public:

    const std::int64_t value() const
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

   const std::vector<Rc<EnumConstant>>& constants() const
   {
       return _constantDecls;
   }

protected:
    Enum()
        : Tag(TypeKind::Enum)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<EnumConstant>> _constantDecls;
    std::set<std::int64_t> _constants;
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

class ParametersDecl : public StructDecl {
public:

protected:
    ParametersDecl() = default;

private:
    friend class Parser;

};

class Component : public Tag {
public:

protected:
    Component()
        : Tag(TypeKind::Component)
    {
    }

private:
    friend class Parser;

    Rc<ParametersDecl> _params;
};
}
