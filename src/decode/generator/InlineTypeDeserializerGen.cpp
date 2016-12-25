#include "decode/generator/InlineTypeDeserializerGen.h"

namespace decode {

InlineTypeDeserializerGen::InlineTypeDeserializerGen(SrcBuilder* output)
    : InlineTypeInspector<InlineTypeDeserializerGen>(output)
{
}

InlineTypeDeserializerGen::~InlineTypeDeserializerGen()
{
}

void InlineTypeDeserializerGen::inspectPointer(const Type* type)
{
    _output->appendReadableSizeCheck(context(), "sizeof(void*)");
    _output->appendIndent(context());
    appendArgumentName();
    _output->append(" = (");
    appendTypeRepr(type);
    _output->append(")PhotonReader_ReadPtr(src);\n");
}

void InlineTypeDeserializerGen::inspectNonInlineType(const Type* type)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&, this, type](SrcBuilder* output) {
        appendTypeRepr(type);
        output->append("_Deserialize(&");
        appendArgumentName();
        output->append(", src)");
    });
}

void InlineTypeDeserializerGen::genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
    _output->appendReadableSizeCheck(context(), sizeCheck);
    _output->appendIndent(context());
    appendArgumentName();
    _output->append(" = PhotonReader_Read");
    _output->append(suffix);
    _output->append("(src);\n");
}

void InlineTypeDeserializerGen::genVarSer(bmcl::StringView suffix)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&](SrcBuilder* output) {
        output->append("PhotonReader_Read");
        output->append(suffix);
        output->append("(src, &");
        appendArgumentName();
        output->append(")");
    });
}
}
