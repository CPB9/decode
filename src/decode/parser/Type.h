#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"

#include <cstdint>

namespace decode {
namespace parser {

enum class BuiltinTypeKind {
    Unknown,
    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
};

enum class ReferenceType {
    None,
    Pointer,
    Reference
};

class Type : public RefCountable {
public:

protected:
    Type() = default;

private:
    friend class Parser;

    ReferenceType _referenceType;
    bool _isMutable;
};

class ArrayType : public Type {
public:
protected:
    ArrayType() = default;

private:
    friend class Parser;

    std::uintmax_t _elementCount;
    Rc<Type> _elementType;
};

class ImportedType : public Type {

protected:
    ImportedType() = default;

private:
    friend class Parser;
    bmcl::StringView _importPath;
    bmcl::StringView _name;
};

}
}
