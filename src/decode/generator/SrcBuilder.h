#pragma once

#include "decode/generator/StringBuilder.h"
#include "decode/generator/InlineSerContext.h"

#include <functional>

namespace decode {

class SrcBuilder;

typedef std::function<void(SrcBuilder*)> SrcGen;

class SrcBuilder : public StringBuilder {
public:
    void setModName(bmcl::StringView modName);

    void appendModPrefix();
    void appendIndent(std::size_t n = 1);
    void appendIndent(const InlineSerContext& ctx);
    void appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size);
    void appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size);
    void appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize);
    void appendWithTryMacro(const SrcGen& func);
    void appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix = bmcl::StringView::empty());

    template <typename... A>
    void appendInclude(A&&... args);

private:
    bmcl::StringView _modName;
};

inline void SrcBuilder::setModName(bmcl::StringView modName)
{
    _modName = modName;
}

inline void SrcBuilder::appendModPrefix()
{
    append("Photon");
    appendWithFirstUpper(_modName);
}

template <typename... A>
void SrcBuilder::appendInclude(A&&... args)
{
    append("#include <");
    append(std::forward<A>(args)...);
    append(">\n");
}

inline void SrcBuilder::appendIndent(std::size_t n)
{
    appendSeveral(n, "    ");
}

inline void SrcBuilder::appendIndent(const InlineSerContext& ctx)
{
    appendSeveral(ctx.indentLevel, "    ");
}

inline void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size)
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

inline void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size)
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

inline void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize)
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

inline void SrcBuilder::appendWithTryMacro(const SrcGen& func)
{
    append("PHOTON_TRY(");
    func(this);
    append(");\n");
}

inline void SrcBuilder::appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix)
{
    append(prefix);
    append(typeName);
    append(' ');
    appendWithFirstLower(varName);
    append(";\n");
}
}
