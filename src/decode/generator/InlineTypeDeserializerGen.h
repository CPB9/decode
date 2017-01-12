#pragma once

#include "decode/Config.h"
#include "decode/core/Target.h"
#include "decode/generator/InlineTypeInspector.h"

namespace decode {

class InlineTypeDeserializerGen : public InlineTypeInspector<InlineTypeDeserializerGen> {
public:
    InlineTypeDeserializerGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output);
    ~InlineTypeDeserializerGen();

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);
};
}
