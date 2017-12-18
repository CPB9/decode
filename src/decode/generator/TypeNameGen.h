/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"

#include <string>

namespace decode {

class SrcBuilder;

class TypeNameGen : public NameVisitor<TypeNameGen> {
public:
    TypeNameGen(SrcBuilder* dest);
    ~TypeNameGen();

    void genTypeName(const Type* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitDynArrayType(const DynArrayType* type);
    bool visitGenericInstantiationType(const GenericInstantiationType* type);
    bool visitFunctionType(const FunctionType* type);

    bool appendTypeName(const NamedType* type);

private:
    SrcBuilder* _output;
};
}
