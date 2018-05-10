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
#include "decode/generator/FuncPrototypeGen.h"
#include "decode/generator/InlineTypeInspector.h"

#include <vector>

namespace decode {

struct ComponentAndMsg;
class TypeReprGen;
class SrcBuilder;
class Project;
class VarRegexp;
class Type;

class StatusEncoderGen {
public:
    StatusEncoderGen(SrcBuilder* output);
    ~StatusEncoderGen();

    void generateStatusDecoderHeader(const Project* project);
    void generateStatusDecoderSource(const Project* project);

    void generateStatusEncoderSource(const Project* project);

    void generateEventEncoderSource(const Project* project);

    void generateAutosaveSource(const Project* project);

private:
    void appendInlineSerializer(const VarRegexp * part, SrcBuilder* currentField, bool isSerializer);
    template <typename T>
    void appendMsgSwitch(const Component* comp, const T* msg);

    SrcBuilder* _output;
    InlineTypeInspector _inlineInspector;
    FuncPrototypeGen _prototypeGen;
};
}
