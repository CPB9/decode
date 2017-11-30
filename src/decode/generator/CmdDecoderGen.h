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

#include <vector>
#include <functional>

namespace decode {

struct ComponentAndMsg;
class TypeReprGen;
class Function;
class SrcBuilder;
class Type;

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
        paramName.resize(2);
    }

    void beginField(const Field*)
    {
        paramName.appendNumericValue(paramIndex);
    }

    void endField(const Field*)
    {
        paramName.resize(2);
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
    void appendFunctionPrototype(unsigned componenNum, unsigned cmdNum);
    void appendMainFunctionPrototype();
    void appendFunctionName(unsigned componenNum, unsigned cmdNum);

    template <typename C>
    void foreachParam(const Function* func, C&& callable);

    void generateFunc(const Component* comp, const Function* func, unsigned componenNum, unsigned cmdNum);
    void generateMainFunc(ComponentMap::ConstRange comps);

    void writePointerOp(const Type* type);
    void writeReturnOp(const Type* type);

    SrcBuilder* _output;
    InlineTypeInspector _inlineInspector;
    InlineCmdParamInspector _paramInspector;
};
}
