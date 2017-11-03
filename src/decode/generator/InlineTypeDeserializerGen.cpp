/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/InlineTypeDeserializerGen.h"

namespace decode {

InlineTypeDeserializerGen::InlineTypeDeserializerGen(TypeReprGen* reprGen, SrcBuilder* output)
    : OnboardInlineTypeInspector<InlineTypeDeserializerGen>(output)
    , _reprGen(reprGen)
{
}

InlineTypeDeserializerGen::~InlineTypeDeserializerGen()
{
}

void InlineTypeDeserializerGen::appendSizeCheck(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest)
{
    dest->appendReadableSizeCheck(ctx, name);
}

void InlineTypeDeserializerGen::inspectPointer(const Type* type)
{
    if (isSizeCheckEnabled()) {
        _output->appendReadableSizeCheck(context(), "sizeof(void*)");
    }
    _output->appendIndent(context());
    appendArgumentName();
    _output->append(" = (");
    _reprGen->genOnboardTypeRepr(type);
    _output->append(")PhotonReader_ReadPtrLe(src);\n");
}

void InlineTypeDeserializerGen::inspectNonInlineType(const Type* type)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&, this, type](SrcBuilder* output) {
        _reprGen->genOnboardTypeRepr(type);
        output->append("_Deserialize(&");
        appendArgumentName();
        output->append(", src)");
    });
}

void InlineTypeDeserializerGen::genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
    if (isSizeCheckEnabled()) {
        _output->appendReadableSizeCheck(context(), sizeCheck);
    }
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
