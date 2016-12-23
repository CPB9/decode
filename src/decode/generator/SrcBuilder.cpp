#include "decode/generator/SrcBuilder.h"

namespace decode {

void SrcBuilder::appendModPrefix()
{
    append("Photon");
    appendWithFirstUpper(_modName);
}

void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendIndent(ctx);
    append("if (PhotonReader_ReadableSize(src) < ");
    appendNumericValue(size);
    append(") {\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughData;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendIndent(ctx);
    append("if (PhotonWriter_WritableSize(dest) < ");
    appendNumericValue(size);
    append(") {\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughSpace;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize)
{
    appendIndent(ctx);
    append("for (size_t ");
    assert(ctx.indentLevel < 30);
    append(ctx.currentLoopVar());
    append(" = 0; ");
    append(ctx.currentLoopVar());
    append(" < ");
    appendNumericValue(loopSize);
    append("; ");
    append(ctx.currentLoopVar());
    append("++) {\n");
}

void SrcBuilder::appendWithTryMacro(const SrcGen& func)
{
    append("PHOTON_TRY(");
    func(this);
    append(");\n");
}

void SrcBuilder::appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix)
{
    append(prefix);
    append(typeName);
    append(' ');
    appendWithFirstLower(varName);
    append(";\n");
}

void SrcBuilder::appendLocalIncludePath(bmcl::StringView path)
{
    append("#include \"photon/");
    append(path);
    append(".h\"\n");
}

void SrcBuilder::appendTagHeader(bmcl::StringView name)
{
    append("typedef ");
    append(name.begin(), name.end());
    append(" {\n");
}

void SrcBuilder::appendTagFooter(bmcl::StringView typeName)
{
    append("} ");
    appendModPrefix();
    append(typeName);
    append(";\n");
}
}
