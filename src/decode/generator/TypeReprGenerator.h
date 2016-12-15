#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>
#include <deque>

namespace decode {

class TypeReprGenerator : public NameVisitor<TypeReprGenerator> {
public:
    TypeReprGenerator(SrcBuilder* dest);
    ~TypeReprGenerator();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    bool visitBuiltin(const BuiltinType* type);
    bool visitArray(const ArrayType* type);
    bool visitReference(const ReferenceType* type);
    bool visitSlice(const SliceType* type);
    bool visitFunction(const Function* type);

    bool appendTypeName(const Type* type);
private:
    void genFnPointerTypeRepr(const Function* type);

    bool hasPrefix;
    SrcBuilder typeName;
    std::deque<bool> pointers; // isConst
    std::string arrayIndices;
    bmcl::StringView fieldName;
};
}
