#pragma once

#include "decode/Config.h"
#include "decode/core/Target.h"
#include "decode/generator/InlineTypeInspector.h"

namespace decode {

class InlineTypeSerializerGen : public InlineTypeInspector<InlineTypeSerializerGen> {
public:
    InlineTypeSerializerGen(const Target* target, SrcBuilder* output);
    ~InlineTypeSerializerGen();

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(std::size_t pointerSize, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);
};

}
