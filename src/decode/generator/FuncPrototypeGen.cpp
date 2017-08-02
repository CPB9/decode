/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/FuncPrototypeGen.h"
#include "decode/ast/Type.h"
#include "decode/ast/Component.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"

namespace decode {

FuncPrototypeGen::FuncPrototypeGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _dest(output)
{
}

FuncPrototypeGen::~FuncPrototypeGen()
{
}

void FuncPrototypeGen::appendSerializerFuncDecl(const Type* type)
{
    _dest->append("PhotonError ");
    _typeReprGen->genTypeRepr(type);
    _dest->append("_Serialize(const ");
    _typeReprGen->genTypeRepr(type);
    if (type->typeKind() != TypeKind::Enum) {
        _dest->append('*');
    }
    _dest->append(" self, PhotonWriter* dest)");
}

void FuncPrototypeGen::appendDeserializerFuncDecl(const Type* type)
{
    _dest->append("PhotonError ");
    _typeReprGen->genTypeRepr(type);
    _dest->append("_Deserialize(");
    _typeReprGen->genTypeRepr(type);
    _dest->append("* self, PhotonReader* src)");
}

void FuncPrototypeGen::appendStatusMessageGenFuncName(const Component* comp, std::uintmax_t msgNum)
{
    _dest->append("_Photon");
    _dest->appendWithFirstUpper(comp->moduleName());
    _dest->append("_GenerateMsg");
    _dest->appendNumericValue(msgNum);
}

void FuncPrototypeGen::appendStatusMessageGenFuncDecl(const Component* comp, std::uintmax_t msgNum)
{
    _dest->append("PhotonError ");
    appendStatusMessageGenFuncName(comp, msgNum);
    _dest->append("(PhotonWriter* dest)");
}
}
