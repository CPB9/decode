#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>
#include <deque>

namespace decode {

class TypeReprGen : public NameVisitor<TypeReprGen> {
public:
    TypeReprGen(SrcBuilder* dest);
    ~TypeReprGen();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitSliceType(const SliceType* type);
    bool visitFunctionType(const FunctionType* type);

    bool appendTypeName(const Type* type);
private:
    void genFnPointerTypeRepr(const FunctionType* type);

    bool hasPrefix;
    SrcBuilder typeName;
    std::deque<bool> pointers; // isConst
    std::string arrayIndices;
    bmcl::StringView fieldName;
};
}
