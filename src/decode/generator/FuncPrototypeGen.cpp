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

void FuncPrototypeGen::appendCmdDecoderFunctionPrototype(const Component* comp, const Command* cmd)
{
    _output->append("PhotonError ");
    appendCmdDecoderFunctionName(comp, cmd);
    _output->append("(PhotonReader* src, PhotonWriter* dest)");
}

void FuncPrototypeGen::appendCmdDecoderFunctionName(const Component* comp, const Command* cmd)
{
    _output->append("Photon");
    _output->appendWithFirstUpper(comp->name());
    _output->append("_DeserializeAndExecCmd_");
    _output->appendWithFirstUpper(cmd->name());
}


template <typename T>
void FuncPrototypeGen::appendWrappedArgs(T range, TypeReprGen* reprGen)
{
    foreachList(range, [&](const Field* arg) {
        Rc<Type> type = wrapPassedTypeIntoPointerIfRequired(const_cast<Type*>(arg->type())); //HACK
        reprGen->genOnboardTypeRepr(type.get(), arg->name());
    }, [this](const Field*) {
        _output->append(", ");
    });
}

void FuncPrototypeGen::appendCmdEncoderFunctionPrototype(const Component* comp, const Command* cmd, TypeReprGen* reprGen)
{
    _output->append("PhotonError Photon");
    _output->appendWithFirstUpper(comp->name());
    _output->append("_SerializeCmd_");
    _output->appendWithFirstUpper(cmd->name());
    if (cmd->type()->argumentsRange().isEmpty()) {
        _output->append("(PhotonWriter* dest)");
    } else {
        _output->append("(");
        appendWrappedArgs(cmd->type()->argumentsRange(), reprGen);
        _output->append(", PhotonWriter* dest)");
    }
}

void FuncPrototypeGen::appendEventEncoderFuncPrototype(const Component* comp, const EventMsg* msg, TypeReprGen* reprGen)
{
   _output->append("PhotonError Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_QueueEvent_");
    _output->appendWithFirstUpper(msg->name());
    _output->append("(");
    appendWrappedArgs(msg->partsRange(), reprGen);
    _output->append(")");
}


void FuncPrototypeGen::appendCmdHandlerFunctionName(const Component* comp, const Command* cmd)
{
    _output->append("Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_ExecCmd_");
    _output->appendWithFirstUpper(cmd->name());
}

void FuncPrototypeGen::appendCmdHandlerFunctionProrotype(const Component* comp, const Command* cmd, TypeReprGen* reprGen)
{
    const FunctionType* ftype = cmd->type();
    _output->append("PhotonError ");
    appendCmdHandlerFunctionName(comp, cmd);
    _output->append("(");

    appendWrappedArgs(ftype->argumentsRange(), reprGen);

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

void FuncPrototypeGen::appendTypeSerializerFunctionPrototype(const Type* type)
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

void FuncPrototypeGen::appendTypeDeserializerFunctionPrototype(const Type* type)
{
    TypeReprGen reprGen(_output);
    _output->append("PhotonError ");
    reprGen.genOnboardTypeRepr(type);
    _output->append("_Deserialize(");
    reprGen.genOnboardTypeRepr(type);
    _output->append("* self, PhotonReader* src)");
}

void FuncPrototypeGen::appendStatusEncoderFunctionName(const Component* comp, const StatusMsg* msg)
{
    _output->append("Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_SerializeStatus_");
    _output->appendWithFirstUpper(msg->name());
}

void FuncPrototypeGen::appendStatusEncoderFunctionPrototype(const Component* comp, const StatusMsg* msg)
{
    _output->append("PhotonError ");
    appendStatusEncoderFunctionName(comp, msg);
    _output->append("(PhotonWriter* dest)");
}

void FuncPrototypeGen::appendStatusDecoderFunctionPrototype(const Component* comp, const StatusMsg* msg)
{
    _output->append("PhotonError ");
    appendStatusDecoderFunctionName(comp, msg);
    _output->append("(PhotonReader* src, Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_StatusMsg_");
    _output->appendWithFirstUpper(msg->name());
    _output->append("* dest)");
}

void FuncPrototypeGen::appendStatusDecoderFunctionName(const Component* comp, const StatusMsg* msg)
{
    _output->append("Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_DeserializeStatus_");
    _output->appendWithFirstUpper(msg->name());
}
}
