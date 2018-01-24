/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/SrcBuilder.h"

#include <bmcl/StringView.h>

namespace decode {

SrcBuilder::~SrcBuilder()
{
}

void SrcBuilder::appendModIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_MODULE_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendDeviceIfDef(bmcl::StringView name)
{
    append("#ifdef PHOTON_DEVICE_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendTargetDeviceIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_DEVICE_TARGET_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendTargetModIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_CMD_TARGET_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendSourceDeviceIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_DEVICE_SOURCE_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendSourceModIfdef(bmcl::StringView name)
{
    append("#ifdef PHOTON_HAS_TM_SOURCE_");
    appendUpper(name);
    append("\n");
}

void SrcBuilder::appendEndif()
{
    append("#endif\n");
}

void SrcBuilder::appendModPrefix(bmcl::StringView name)
{
    append("Photon");
    appendWithFirstUpper(name);
}

void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck)
{
    appendIndent(ctx);
    append("if (PhotonReader_ReadableSize(src) < ");
    append(sizeCheck);
    append(") {\n");
    appendIndent(ctx);
    append("    PHOTON_CRITICAL(\"Not enough data to deserialize\");\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughData;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck)
{
    appendIndent(ctx);
    append("if (PhotonWriter_WritableSize(dest) < ");
    append(sizeCheck);
    append(") {\n");
    appendIndent(ctx);
    append("    PHOTON_DEBUG(\"Not enough space to serialize\");\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughSpace;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendReadableSizeCheck(ctx, std::to_string(size));
}

void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendWritableSizeCheck(ctx, std::to_string(size));
}

void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, bmcl::StringView loopSize)
{
    appendIndent(ctx);
    append("for (size_t ");
    assert(ctx.indentLevel < 30);
    append(ctx.currentLoopVar());
    append(" = 0; ");
    append(ctx.currentLoopVar());
    append(" < ");
    append(loopSize);
    append("; ");
    append(ctx.currentLoopVar());
    append("++) {\n");
}

void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize)
{
    appendLoopHeader(ctx, std::to_string(loopSize));
}

void SrcBuilder::appendIndent()
{
    append("    ");
}

void SrcBuilder::appendIndent(std::size_t n)
{
    appendSeveral(n, "    ");
}

void SrcBuilder::appendIndent(const InlineSerContext& ctx)
{
    appendSeveral(ctx.indentLevel, "    ");
}

void SrcBuilder::appendWithTryMacro(const SrcGen& func)
{
    append("PHOTON_TRY(");
    func(this);
    append(");\n");
}

void SrcBuilder::appendWithTryMacro(const SrcGen& func, bmcl::StringView msg)
{
    append("PHOTON_TRY_MSG(");
    func(this);
    append(", \"");
    append(msg);
    append("\");\n");
}

void SrcBuilder::appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName)
{
    appendVarDecl(typeName, varName, bmcl::StringView::empty());
}

void SrcBuilder::appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix)
{
    append(prefix);
    append(typeName);
    append(' ');
    appendWithFirstLower(varName);
    append(";\n");
}

void SrcBuilder::appendOnboardIncludePath(bmcl::StringView path)
{
    append("#include \"photongen/onboard/");
    append(path);
    append(".h\"\n");
}

void SrcBuilder::appendTagHeader(bmcl::StringView name)
{
    append("typedef ");
    append(name);
    append(" {\n");
}

void SrcBuilder::appendTagFooter(bmcl::StringView name)
{
    append("} Photon");
    appendWithFirstUpper(name);
    append(";\n");
}

void SrcBuilder::startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName)
{
    auto writeGuardMacro = [this, typeName, modName]() {
        append("__PHOTON_");
        appendUpper(modName);
        append('_');
        appendUpper(typeName);
        append("_H__\n");
    };
    append("#ifndef ");
    writeGuardMacro();
    append("#define ");
    writeGuardMacro();
    appendEol();
}

void SrcBuilder::startCppGuard()
{
    append("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");
}

void SrcBuilder::endCppGuard()
{
    append("#ifdef __cplusplus\n}\n#endif\n\n");
}

void SrcBuilder::endIncludeGuard()
{
    append("#endif\n");
    appendEol();
}

void SrcBuilder::appendByteArrayDefinition(bmcl::StringView prefix, bmcl::StringView name, bmcl::Bytes data)
{
    append(prefix);
    if (prefix.isEmpty()) {
        append("uint8_t ");
    } else {
        append(" uint8_t ");
    }
    append(name);
    append('[');
    appendNumericValue(data.size());
    append("] = {");
    std::size_t size;
    const std::size_t maxBytes = 12;
    for (size = 0; size < data.size(); size++) {
        if ((size % maxBytes) == 0) {
            append("\n   ");
        }
        uint8_t b = data[size];
        append(" ");
        appendHexValue(b);
        append(",");
    }
    append("\n};\n");

}

void SrcBuilder::appendOnboardComponentInclude(bmcl::StringView name, bmcl::StringView ext)
{
    append("#include \"photongen/onboard/");
    append(name);
    append("/");
    appendWithFirstUpper(name);
    append(".Component");
    append(ext);
    append("\"\n");
}

void SrcBuilder::appendImplIncludePath(bmcl::StringView path)
{
    append("#include \"photon/");
    append(path);
    append(".h\"\n");
}

void SrcBuilder::appendPragmaOnce()
{
    append("#pragma once\n");
}
}
