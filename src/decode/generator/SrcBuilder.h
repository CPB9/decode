/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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
    void appendReadableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck);
    void appendWritableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck);
    void appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize);
    void appendLoopHeader(const InlineSerContext& ctx, bmcl::StringView loopSize);
    void appendWithTryMacro(const SrcGen& func);
    void appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix = bmcl::StringView::empty());
    void appendLocalIncludePath(bmcl::StringView path);
    void appendTagHeader(bmcl::StringView name);
    void appendTagFooter(bmcl::StringView name);
    void appendByteArrayDefinition(bmcl::StringView prefix, bmcl::StringView name, bmcl::Bytes data);
    void appendTypeInclude(bmcl::StringView name, bmcl::StringView ext = ".h");
    void appendComponentInclude(bmcl::StringView name, bmcl::StringView ext = ".h");
    void startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName);
    void startCppGuard();
    void endIncludeGuard();
    void endCppGuard();
    void appendModIfdef(bmcl::StringView name);
    void appendDeviceIfDef(bmcl::StringView name);
    void appendTargetModIfdef(bmcl::StringView name);
    void appendEndif();

    template <typename T>
    void appendNumericValueDefine(bmcl::StringView name, T value);

    template <typename... A>
    void appendInclude(A&&... args);

    bmcl::StringView modName() const;

private:
    bmcl::StringView _modName; //TODO: remove
};

inline void SrcBuilder::setModName(bmcl::StringView modName)
{
    _modName = modName;
}

inline bmcl::StringView SrcBuilder::modName() const
{
    return _modName;
}

inline void SrcBuilder::appendModIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_MODULE_");
    appendUpper(name);
    append("\n");
}

inline void SrcBuilder::appendDeviceIfDef(bmcl::StringView name)
{
    append("#ifdef PHOTON_DEVICE_");
    appendUpper(name);
    append("\n");
}

inline void SrcBuilder::appendTargetModIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_TARGET_MODULE_");
    appendUpper(name);
    append("\n");
}

inline void SrcBuilder::appendEndif()
{
    append("#endif\n");
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

template <typename T>
void SrcBuilder::appendNumericValueDefine(bmcl::StringView name, T value)
{
    append("#define ");
    append(name);
    append(' ');
    appendNumericValue(value);
    append("\n");
}
}
