/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/StringBuilder.h"
#include "decode/generator/InlineSerContext.h"

#include <bmcl/StringView.h>

#include <functional>

namespace decode {

class SrcBuilder;

typedef std::function<void(SrcBuilder*)> SrcGen;

class SrcBuilder : public StringBuilder {
public:
    template <typename... A>
    SrcBuilder(A&&... args);
    ~SrcBuilder();

    void appendModPrefix(bmcl::StringView name);
    void appendIndent();
    void appendIndent(std::size_t n);
    void appendIndent(const InlineSerContext& ctx);
    void appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size);
    void appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size);
    void appendReadableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck);
    void appendWritableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck);
    void appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize);
    void appendLoopHeader(const InlineSerContext& ctx, bmcl::StringView loopSize);
    void appendWithTryMacro(const SrcGen& func);
    void appendWithTryMacro(const SrcGen& func, bmcl::StringView msg);
    void appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix = bmcl::StringView::empty());
    void appendOnboardIncludePath(bmcl::StringView path);
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
    void appendTargetDeviceIfdef(bmcl::StringView name);
    void appendSourceDeviceIfdef(bmcl::StringView name);
    void appendTargetModIfdef(bmcl::StringView name);
    void appendSourceModIfdef(bmcl::StringView name);
    void appendEndif();
    void appendPragmaOnce();

    template <typename T>
    void appendNumericValueDefine(bmcl::StringView name, T value);

    template <typename... A>
    void appendInclude(A&&... args);
};

template <typename... A>
SrcBuilder::SrcBuilder(A&&... args)
    : StringBuilder(std::forward<A>(args)...)
{
}

template <typename... A>
void SrcBuilder::appendInclude(A&&... args)
{
    append("#include <");
    append(std::forward<A>(args)...);
    append(">\n");
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
