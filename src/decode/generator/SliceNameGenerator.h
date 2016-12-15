#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class SliceNameGenerator : public NameVisitor<SliceNameGenerator> {
public:
    SliceNameGenerator(SrcBuilder* dest);

    void genSliceName(const SliceType* type);

    bool visitBuiltin(const BuiltinType* type);
    bool visitArray(const ArrayType* type);
    bool visitReference(const ReferenceType* type);
    bool visitSlice(const SliceType* type);

    bool appendTypeName(const Type* type);
};
}
