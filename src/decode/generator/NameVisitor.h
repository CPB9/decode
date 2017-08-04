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

namespace decode {

template <typename B>
class NameVisitor : public ConstAstVisitor<B> {
public:
    B& base();

    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);
    bool visitAliasType(const AliasType* type);

protected:
    bool appendTypeName(const Type* type);
};

template <typename B>
inline B& NameVisitor<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
inline bool NameVisitor<B>::visitEnumType(const EnumType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitStructType(const StructType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitVariantType(const VariantType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitImportedType(const ImportedType* type)
{
    return base().appendTypeName(type->link());
}

template <typename B>
inline bool NameVisitor<B>::visitAliasType(const AliasType* type)
{
    return base().appendTypeName(type);
}
}
