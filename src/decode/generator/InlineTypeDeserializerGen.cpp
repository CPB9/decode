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
    : InlineTypeInspector<InlineTypeDeserializerGen>(reprGen, output)
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
    _output->append(")PhotonReader_ReadPtrLe(src);\n");
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
