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
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>
#include <vector>

namespace decode {

class TypeReprGen : public RefCountable {
public:
    using Pointer = Rc<TypeReprGen>;
    using ConstPointer = Rc<const TypeReprGen>;

    TypeReprGen(SrcBuilder* dest);
    ~TypeReprGen();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

private:
    void writeBuiltin(const BuiltinType* type);
    void writeArray(const ArrayType* type);
    void writePointer(const ReferenceType* type);
    void writeNamed(const Type* type);
    void writeFunction(const FunctionType* type);
    void writeType(const Type* type);

    std::size_t _currentOffset;
    SrcBuilder* _output;
    SrcBuilder _temp;
};
}
