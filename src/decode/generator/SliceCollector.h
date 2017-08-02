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
#include "decode/parser/Containers.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class Type;
class TypeReprGen;
class Component;
class SliceType;

class SliceCollector : public ConstAstVisitor<SliceCollector> {
public:
    using NameToSliceMap = RcSecondUnorderedMap<std::string, const SliceType>;

    SliceCollector();
    ~SliceCollector();

    void collectUniqueSlices(const Type* type, NameToSliceMap* dest);
    void collectUniqueSlices(const Component* type, NameToSliceMap* dest);

    bool visitSliceType(const SliceType* slice);

private:
    SrcBuilder _sliceName;
    NameToSliceMap* _dest;
};
}
