/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/InlineTypeInspector.h"

namespace decode {

class GcInlineTypeSerializerGen : public InlineTypeInspector<GcInlineTypeSerializerGen> {
public:
    GcInlineTypeSerializerGen(SrcBuilder* output);
    ~GcInlineTypeSerializerGen();

    static void appendSizeCheck(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);

    bool visitBuiltinType(const BuiltinType* type);
    void inspectDynArrayType(const DynArrayType* type);
};


inline GcInlineTypeSerializerGen::GcInlineTypeSerializerGen(SrcBuilder* output)
    : InlineTypeInspector<decode::GcInlineTypeSerializerGen>(output)
{
}

inline GcInlineTypeSerializerGen::~GcInlineTypeSerializerGen()
{
}

inline void GcInlineTypeSerializerGen::appendSizeCheck(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest)
{
}

inline void GcInlineTypeSerializerGen::inspectPointer(const Type* type)
{
}

inline void GcInlineTypeSerializerGen::inspectNonInlineType(const Type* type)
{
}

inline void GcInlineTypeSerializerGen::genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
}

inline void GcInlineTypeSerializerGen::genVarSer(bmcl::StringView suffix)
{
}

inline bool GcInlineTypeSerializerGen::visitBuiltinType(const BuiltinType* type)
{
    return false;
}

inline void GcInlineTypeSerializerGen::inspectDynArrayType(const DynArrayType* type)
{
}
}

