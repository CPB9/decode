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
class EventMsg;
class Component;
class Command;
class StatusMsg;
class CmdArgument;

class FuncPrototypeGen {
public:
    FuncPrototypeGen(SrcBuilder* output);
    ~FuncPrototypeGen();

    void appendCmdDecoderFunctionPrototype(const Component* comp, const Command* cmd);
    void appendCmdDecoderFunctionName(const Component* comp, const Command* cmd);
    void appendCmdEncoderFunctionPrototype(const Component* comp, const Command* cmd, TypeReprGen* reprGen);
    void appendEventEncoderFunctionPrototype(const Component* comp, const EventMsg* msg, TypeReprGen* reprGen);
    void appendEventDecoderFunctionPrototype(const Component* comp, const EventMsg* msg);
    void appendEventDecoderFunctionName(const Component* comp, const EventMsg* msg);
    void appendCmdHandlerFunctionProrotype(const Component* comp, const Command* cmd, TypeReprGen* reprGen);
    void appendCmdHandlerFunctionName(const Component* comp, const Command* cmd);
    void appendCmdArgAllocFunctionName(const Component* comp, const Command* cmd, const CmdArgument& arg);
    void appendCmdArgAllocFunctionPrototype(const Component* comp, const Command* cmd, const CmdArgument& arg, TypeReprGen* reprGen);
    void appendTypeDeserializerFunctionPrototype(const Type* type);
    void appendTypeSerializerFunctionPrototype(const Type* type);
    void appendStatusEncoderFunctionPrototype(const Component* comp, const StatusMsg* msg);
    void appendStatusEncoderFunctionName(const Component* comp, const StatusMsg* msg);
    void appendStatusDecoderFunctionPrototype(const Component* comp, const StatusMsg* msg);
    void appendStatusDecoderFunctionName(const Component* comp, const StatusMsg* msg);

private:
    template <typename T>
    void appendWrappedFuncArgs(T range, TypeReprGen* reprGen);

    template <typename T>
    void appendWrappedCmdArgs(T range, TypeReprGen* reprGen);

    SrcBuilder* _output;
};

}
