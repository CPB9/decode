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

#include <cstdint>

namespace decode {

class SrcBuilder;
class TypeReprGen;
class Type;
class Component;

class FuncPrototypeGen {
public:
    FuncPrototypeGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~FuncPrototypeGen();

    void appendDeserializerFuncDecl(const Type* type);
    void appendSerializerFuncDecl(const Type* type);
    void appendStatusMessageGenFuncDecl(const Component* comp, std::uintmax_t msgNum);
    void appendStatusMessageGenFuncName(const Component* comp, std::uintmax_t msgNum);

private:
    Rc<TypeReprGen> _typeReprGen;
    SrcBuilder* _dest;
};

}
