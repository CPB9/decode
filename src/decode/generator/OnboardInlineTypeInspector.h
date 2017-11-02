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

template <typename B>
class OnboardInlineTypeInspector : public InlineTypeInspector<B> {
public:
    OnboardInlineTypeInspector(SrcBuilder* output);

    bool visitBuiltinType(const BuiltinType* type);
    void inspectDynArrayType(const DynArrayType* type);

    B& base();
};

template <typename B>
inline OnboardInlineTypeInspector<B>::OnboardInlineTypeInspector(SrcBuilder* output)
    : InlineTypeInspector<B>(output)
{
}

template <typename B>
inline B& OnboardInlineTypeInspector<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
inline void OnboardInlineTypeInspector<B>::inspectDynArrayType(const DynArrayType* type)
{
    base().inspectNonInlineType(type);
}

template <typename B>
bool OnboardInlineTypeInspector<B>::visitBuiltinType(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        base().genSizedSer("sizeof(void*)", "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        base().genSizedSer("sizeof(void*)", "USizeLe");
        break;
    case BuiltinTypeKind::U8:
        base().genSizedSer("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::I8:
        base().genSizedSer("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::U16:
        base().genSizedSer("sizeof(uint16_t)", "U16Le");
        break;
    case BuiltinTypeKind::I16:
        base().genSizedSer("sizeof(uint16_t)", "U16Le");
        break;
    case BuiltinTypeKind::U32:
        base().genSizedSer("sizeof(uint32_t)", "U32Le");
        break;
    case BuiltinTypeKind::I32:
        base().genSizedSer("sizeof(uint32_t)", "U32Le");
        break;
    case BuiltinTypeKind::U64:
        base().genSizedSer("sizeof(uint64_t)", "U64Le");
        break;
    case BuiltinTypeKind::I64:
        base().genSizedSer("sizeof(uint64_t)", "U64Le");
        break;
    case BuiltinTypeKind::F32:
        base().genSizedSer("sizeof(float)", "F32Le");
        break;
    case BuiltinTypeKind::F64:
        base().genSizedSer("sizeof(double)", "F64Le");
        break;
    case BuiltinTypeKind::Bool:
        base().genSizedSer("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::Char:
        base().genSizedSer("sizeof(char)", "Char");
        break;
    case BuiltinTypeKind::Varuint:
        base().genVarSer("Varuint");
        break;
    case BuiltinTypeKind::Varint:
        base().genVarSer("Varint");
        break;
    case BuiltinTypeKind::Void:
        //TODO: disallow
        assert(false);
        break;
    default:
        assert(false);
    }
    return false;
}
}
