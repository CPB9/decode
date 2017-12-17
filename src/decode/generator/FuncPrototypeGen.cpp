/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/FuncPrototypeGen.h"
#include "decode/core/Foreach.h"
#include "decode/ast/Type.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/ast/Field.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/Utils.h"

namespace decode {

FuncPrototypeGen::FuncPrototypeGen(SrcBuilder* output)
    : _output(output)
{
}

FuncPrototypeGen::~FuncPrototypeGen()
{
}

void FuncPrototypeGen::appendEventFuncDecl(const Component* comp, const EventMsg* msg, TypeReprGen* reprGen)
{
   _output->append("PhotonError Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_QueueEvent_");
    _output->appendWithFirstUpper(msg->name());
    _output->append("(");
    foreachList(msg->partsRange(), [&](const Field* field) {
        Rc<Type> type = wrapPassedTypeIntoPointerIfRequired(const_cast<Type*>(field->type())); //HACK
        reprGen->genOnboardTypeRepr(type.get(), field->name());
    }, [this](const Field*) {
        _output->append(", ");
    });
    _output->append(")");
}

void FuncPrototypeGen::appendCmdFuncDecl(const Component* comp, const Command* cmd, TypeReprGen* reprGen)
{
    const FunctionType* ftype = cmd->type();
    _output->append("PhotonError Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_");
    _output->appendWithFirstUpper(cmd->name());
    _output->append("(");

    foreachList(ftype->argumentsRange(), [&](const Field* field) {
        Rc<Type> type = wrapPassedTypeIntoPointerIfRequired(const_cast<Type*>(field->type())); //HACK
        reprGen->genOnboardTypeRepr(type.get(), field->name());
    }, [this](const Field*) {
        _output->append(", ");
    });

    auto rv = const_cast<FunctionType*>(ftype)->returnValue(); //HACK
    if (rv.isSome()) {
        if (ftype->hasArguments()) {
            _output->append(", ");
        }
        if (rv->isArray()) {
            reprGen->genOnboardTypeRepr(rv.unwrap(), "rv");
        } else {
            Rc<const ReferenceType> rtype = new ReferenceType(ReferenceKind::Pointer, true, rv.unwrap());  //HACK
            reprGen->genOnboardTypeRepr(rtype.get(), "rv"); //TODO: check name conflicts
        }
    }
    _output->append(")");
}

void FuncPrototypeGen::appendSerializerFuncDecl(const Type* type)
{
    TypeReprGen reprGen(_output);
    _output->append("PhotonError ");
    reprGen.genOnboardTypeRepr(type);
    _output->append("_Serialize(const ");
    reprGen.genOnboardTypeRepr(type);
    if (type->typeKind() != TypeKind::Enum) {
        _output->append('*');
    }
    _output->append(" self, PhotonWriter* dest)");
}

void FuncPrototypeGen::appendDeserializerFuncDecl(const Type* type)
{
    TypeReprGen reprGen(_output);
    _output->append("PhotonError ");
    reprGen.genOnboardTypeRepr(type);
    _output->append("_Deserialize(");
    reprGen.genOnboardTypeRepr(type);
    _output->append("* self, PhotonReader* src)");
}

void FuncPrototypeGen::appendStatusMessageGenFuncName(const Component* comp, const StatusMsg* msg)
{
    _output->append("Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_SerializeStatus_");
    _output->appendWithFirstUpper(msg->name());
}

void FuncPrototypeGen::appendStatusMessageGenFuncDecl(const Component* comp, const StatusMsg* msg)
{
    _output->append("PhotonError ");
    appendStatusMessageGenFuncName(comp, msg);
    _output->append("(PhotonWriter* dest)");
}
}
