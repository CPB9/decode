#include "decode/generator/InlineTypeDeserializerGen.h"

namespace decode {

InlineTypeDeserializerGen::InlineTypeDeserializerGen(const Target* target, SrcBuilder* output)
    : InlineTypeInspector<InlineTypeDeserializerGen>(target, output)
{
}

InlineTypeDeserializerGen::~InlineTypeDeserializerGen()
{
}

void InlineTypeDeserializerGen::inspectPointer(const Type* type)
{
    _output->appendReadableSizeCheck(context(), _target->pointerSize());
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

void InlineTypeDeserializerGen::genSizedSer(std::size_t pointerSize, bmcl::StringView suffix)
{
    _output->appendReadableSizeCheck(context(), pointerSize);
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
