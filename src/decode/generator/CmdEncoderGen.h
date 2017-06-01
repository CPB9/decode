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
#include "decode/generator/InlineTypeSerializerGen.h"

namespace decode {

class TypeReprGen;
class SrcBuilder;
class Component;
class Function;

class CmdEncoderGen {
public:
    CmdEncoderGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~CmdEncoderGen();

    void generateHeader(ComponentMap::ConstRange comps); //TODO: make generic
    void generateSource(ComponentMap::ConstRange comps);

private:
    void appendEncoderPrototype(const Component* comp, const Function* func);

    Rc<TypeReprGen> _reprGen;
    SrcBuilder* _output;
    InlineTypeSerializerGen _inlineSer;
};

}
