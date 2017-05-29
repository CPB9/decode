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

class InlineTypeDeserializerGen : public InlineTypeInspector<InlineTypeDeserializerGen> {
public:
    InlineTypeDeserializerGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~InlineTypeDeserializerGen();

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);
};
}
