#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class SliceNameGen : public NameVisitor<SliceNameGen> {
public:
    SliceNameGen(SrcBuilder* dest);

    void genSliceName(const SliceType* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitSliceType(const SliceType* type);

    bool appendTypeName(const Type* type);
};
}
