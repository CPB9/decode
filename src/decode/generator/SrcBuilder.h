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
    void appendLocalIncludePath(bmcl::StringView path);
    void appendTagHeader(bmcl::StringView name);
    void appendTagFooter(bmcl::StringView name);

    template <typename... A>
    void appendInclude(A&&... args);

private:
    bmcl::StringView _modName;
};

inline void SrcBuilder::setModName(bmcl::StringView modName)
{
    _modName = modName;
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
}
