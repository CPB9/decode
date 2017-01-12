#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class TypeNameGen : public NameVisitor<TypeNameGen> {
public:
    TypeNameGen(StringBuilder* dest);

    static std::string genTypeNameAsString(const Type* type);

    void genTypeName(const Type* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitSliceType(const SliceType* type);

    bool appendTypeName(const NamedType* type);

private:
    StringBuilder* _output;
};
}
