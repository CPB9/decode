/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/InlineSerContext.h"

#include <bmcl/Fwd.h>
#include <bmcl/Option.h>

#include <stack>
#include <vector>

namespace decode {

class Type;
class SrcBuilder;

class ArrayType;
class BuiltinType;
class DynArrayType;
class TypeReprGen;

class InlineTypeInspector {
public:
    InlineTypeInspector(SrcBuilder* output);

    void genOnboardSerializer(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes = true);
    void genOnboardDeserializer(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes = true);

    template <bool isOnboard, bool isSerializer>
    void inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes = true);
    template <bool isOnboard, bool isSerializer>
    static void appendSizeCheck(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);

private:
    const InlineSerContext& context() const;
    bool isSizeCheckEnabled() const;
    void appendArgumentName();
    void popArgName(std::size_t n);

    template <bool isOnboard, bool isSerializer>
    void inspectType(const Type* type);
    template <bool isOnboard, bool isSerializer>
    void inspectPointer(const Type* type);
    template <bool isOnboard, bool isSerializer>
    void inspectNonInlineType(const Type* type);
    template <bool isOnboard, bool isSerializer>
    void inspectArray(const ArrayType* type);

    template <bool isSerializer>
    void inspectGcBuiltin(const BuiltinType* type);
    template <bool isSerializer>
    void inspectGcDynArray(const DynArrayType* type);

    template <bool isSerializer>
    void inspectOnboardBuiltin(const BuiltinType* type);
    template <bool isSerializer>
    void inspectOnboardNonInlineType(const Type* type);
    template <bool isSerializer>
    void genOnboardSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    template <bool isSerializer>
    void genOnboardVarSer(bmcl::StringView suffix);

    template <bool isSerializer>
    void genGcSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    template <bool isSerializer>
    void genGcVarSer(bmcl::StringView suffix);

    void deserializeOnboardPointer(const Type* type);
    void serializeOnboardPointer(const Type* type);
    void deserializeGcPointer(const Type* type);
    void serializeGcPointer(const Type* type);

    SrcBuilder* _output;
    std::stack<InlineSerContext, std::vector<InlineSerContext>> _ctxStack;
    std::string _argName;
    bool _checkSizes;
};

extern template void InlineTypeInspector::inspect<true, true>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
extern template void InlineTypeInspector::inspect<true, false>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
extern template void InlineTypeInspector::inspect<false, true>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
extern template void InlineTypeInspector::inspect<false, false>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
extern template void InlineTypeInspector::appendSizeCheck<true, true>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
extern template void InlineTypeInspector::appendSizeCheck<true, false>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
extern template void InlineTypeInspector::appendSizeCheck<false, true>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
extern template void InlineTypeInspector::appendSizeCheck<false, false>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
}
