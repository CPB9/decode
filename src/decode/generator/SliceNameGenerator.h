#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/generator/StringBuilder.h"

#include <string>

namespace decode {

class SliceNameGenerator : public ConstAstVisitor<SliceNameGenerator> {
public:
    SliceNameGenerator(StringBuilder* dest);
    ~SliceNameGenerator();

    void genSliceName(const SliceType* type);

    bool visitBuiltin(const BuiltinType* type);
    bool visitArray(const ArrayType* type);
    bool visitReference(const ReferenceType* type);
    bool visitSlice(const SliceType* type);
    bool visitFunction(const Function* type);
    bool visitEnum(const Enum* type);
    bool visitStruct(const StructType* type);
    bool visitVariant(const Variant* type);
    bool visitUnresolved(const UnresolvedType* type);

private:
    bmcl::StringView builtinToName(const BuiltinType* type) const;

    bool appendTypeName(const Type* type);

    StringBuilder* _output;
};
}
