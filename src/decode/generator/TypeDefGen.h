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
#include "decode/generator/IncludeCollector.h"

#include <string>

namespace decode {

class Component;
class SrcBuilder;
class TypeReprGen;

class TypeDefGen : public ConstAstVisitor<TypeDefGen> {
public:
    TypeDefGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~TypeDefGen();

    void genTypeDef(const Type* type);
    void genComponentDef(const Component* comp);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitSliceType(const SliceType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);
    bool visitAliasType(const AliasType* type);

private:
    void appendFieldVec(TypeVec::ConstRange fields, bmcl::StringView name);
    void appendFieldVec(FieldVec::ConstRange fields, bmcl::StringView name);
    void appendStruct(const StructType* type);
    void appendEnum(const EnumType* type);
    void appendVariant(const VariantType* type);

    Rc<TypeReprGen> _typeReprGen;
    SrcBuilder* _output;
};
}
