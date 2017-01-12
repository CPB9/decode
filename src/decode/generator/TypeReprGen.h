#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>
#include <vector>

namespace decode {

class TypeReprGen : public NameVisitor<TypeReprGen>, public RefCountable {
public:
    TypeReprGen(SrcBuilder* dest);
    ~TypeReprGen();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitSliceType(const SliceType* type);
    bool visitFunctionType(const FunctionType* type);

    bool appendTypeName(const NamedType* type);
private:
    void genFnPointerTypeRepr(const FunctionType* type);

    bool _hasPrefix;
    SrcBuilder _typeName;
    std::vector<bool> _pointers; // isConst
    std::string _arrayIndices;
    bmcl::StringView _fieldName;
    SrcBuilder* _output;
};
}
