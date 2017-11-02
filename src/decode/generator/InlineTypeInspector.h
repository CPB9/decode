/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/ast/AstVisitor.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"

#include <stack>
#include <vector>

namespace decode {

template <typename B>
class InlineTypeInspector : public ConstAstVisitor<B> {
public:
    InlineTypeInspector(SrcBuilder* output);

    void inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes = true);

    B& base();

    bool visitArrayType(const ArrayType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitDynArrayType(const DynArrayType* type);
    bool visitAliasType(const AliasType* type);
    bool visitImportedType(const ImportedType* type);
    bool visitGenericInstantiationType(const GenericInstantiationType* type);

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);

protected:

    const InlineSerContext& context() const;
    bool isSizeCheckEnabled() const;
    void appendArgumentName();
    void popArgName(std::size_t n);

    std::stack<InlineSerContext, std::vector<InlineSerContext>> _ctxStack;
    SrcBuilder* _output;
    std::string _argName;
    bool _checkSizes;
};

template <typename B>
class InlineFieldInspector {
public:
    InlineFieldInspector(SrcBuilder* dest)
        : _dest(dest)
    {
    }

    B& base()
    {
        return *static_cast<B*>(this);
    }

    template <typename I>
    void inspect(FieldVec::ConstRange fields, I* typeInspector)
    {
        InlineSerContext ctx;
        auto begin = fields.begin();
        auto it = begin;
        auto end = fields.end();

        while (it != end) {
            bmcl::Option<std::size_t> totalSize;
            while (it != end) {
                bmcl::Option<std::size_t> size = it->type()->fixedSize();
                if (size.isNone()) {
                    break;
                }
                totalSize.emplace(totalSize.unwrapOr(0) + size.unwrap());
                it++;
            }
            if (totalSize.isSome()) {
                typeInspector->appendSizeCheck(ctx, std::to_string(totalSize.unwrap()), _dest);
                for (auto jt = begin; jt < it; jt++) {
                    base().beginField(*jt);
                    typeInspector->inspect(jt->type(), ctx, base().currentFieldName(), false);
                    base().endField(*jt);
                }

                totalSize.clear();
            } else {
                base().beginField(*it);
                typeInspector->inspect(it->type(), ctx, base().currentFieldName());
                base().endField(*it);
                it++;
                begin = it;
            }
        }
    }

private:
    SrcBuilder* _dest;
};

template <typename B>
inline bool InlineTypeInspector<B>::isSizeCheckEnabled() const
{
    return _checkSizes;
}

template <typename B>
inline void InlineTypeInspector<B>::appendArgumentName()
{
    _output->append(_argName);
}

template <typename B>
inline void InlineTypeInspector<B>::popArgName(std::size_t n)
{
    _argName.erase(_argName.size() - n, n);
}

template <typename B>
inline const InlineSerContext& InlineTypeInspector<B>::context() const
{
    return _ctxStack.top();
}

template <typename B>
inline B& InlineTypeInspector<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
InlineTypeInspector<B>::InlineTypeInspector(SrcBuilder* output)
    : _output(output)
{
}

template <typename B>
void InlineTypeInspector<B>::inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes)
{
    assert(_ctxStack.size() == 0);
    _ctxStack.push(ctx);
    _argName.assign(argName.begin(), argName.end());
    _checkSizes = checkSizes;
    base().traverseType(type);
    _ctxStack.pop();
}

template <typename B>
inline bool InlineTypeInspector<B>::visitFunctionType(const FunctionType* type)
{
    base().inspectPointer(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitReferenceType(const ReferenceType* type)
{
    base().inspectPointer(type);
    return false;
}

template <typename B>
bool InlineTypeInspector<B>::visitArrayType(const ArrayType* type)
{
    _argName.push_back('[');
    _argName.push_back(context().currentLoopVar());
    _argName.push_back(']');
    bool oldCheckSizes = _checkSizes;
    if (_checkSizes) {
        _checkSizes = false;
        auto size = type->elementType()->fixedSize();
        if (size.isSome()) {
            base().appendSizeCheck(context(), std::to_string(size.unwrap() * type->elementCount()), _output);
        }
    }
    _output->appendLoopHeader(context(), type->elementCount());
    _ctxStack.push(context().indent().incLoopVar());
    base().traverseType(type->elementType());
    _checkSizes = oldCheckSizes;
    popArgName(3);
    _ctxStack.pop();
    _output->appendIndent(context());
    _output->append("}\n");
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitStructType(const StructType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitVariantType(const VariantType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitEnumType(const EnumType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitDynArrayType(const DynArrayType* type)
{
    base().inspectDynArrayType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitAliasType(const AliasType* type)
{
    base().traverseType(type->alias());
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitImportedType(const ImportedType* type)
{
    base().traverseType(type->link());
    return false;
}

}

