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
#include "decode/parser/Containers.h"
#include "decode/generator/InlineTypeInspector.h"
#include "decode/generator/InlineFieldInspector.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/ast/Function.h" //TODO: remove

#include <vector>
#include <functional>

namespace decode {

struct ComponentAndMsg;
class TypeReprGen;
class Function;
class Command;
class SrcBuilder;
class Type;
class CmdArgument;

class InlineCmdParamInspector : public InlineFieldInspector<InlineCmdParamInspector> {
public:
    InlineCmdParamInspector(SrcBuilder* dest)
        : InlineFieldInspector<InlineCmdParamInspector>(dest)
        , paramIndex(0)
        , paramName("_p0")
    {
    }

    void reset()
    {
        paramIndex = 0;
        paramName.resize(0);
    }

    void beginGenericField(const CmdArgument&)
    {
        paramName.append("_p");
        paramName.appendNumericValue(paramIndex);
    }

    void beginField(const CmdArgument& arg)
    {
        if (arg.argPassKind() == CmdArgPassKind::AllocPtr) {
            paramName.append("(*_p");
            paramName.appendNumericValue(paramIndex);
            paramName.append(")");
        } else {
            beginGenericField(arg);
        }
    }

    void endField(const CmdArgument&)
    {
        paramName.resize(0);
        paramIndex++;
    }

    bmcl::StringView currentFieldName() const
    {
        return paramName.view();
    }

    std::size_t paramIndex;
    StringBuilder paramName;
};

class CmdDecoderGen {
public:
    CmdDecoderGen(SrcBuilder* output);
    ~CmdDecoderGen();

    void generateHeader(ComponentMap::ConstRange comps); //TODO: make generic
    void generateSource(ComponentMap::ConstRange comps);

private:
    void appendCmdFunctionPrototype();
    void appendScriptFunctionPrototype();

    template <typename C>
    void foreachParam(const Command* func, C&& callable);

    void generateCmdFunc(ComponentMap::ConstRange comps);
    void generateScriptFunc();
    void generateDecoder(const Component* comp, const Command* cmd);

    void writePointerOp(const Type* type);
    void writeReturnOp(const Type* type);

    SrcBuilder* _output;
    InlineTypeInspector _inlineInspector;
    InlineCmdParamInspector _paramInspector;
};
}
