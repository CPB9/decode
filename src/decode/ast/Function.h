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
#include "decode/core/NamedRc.h"
#include "decode/core/NamedRc.h"
#include "decode/core/CmdArgPassKind.h"
#include "decode/parser/Containers.h"
#include "decode/ast/DocBlockMixin.h"

#include <bmcl/Fwd.h>

namespace decode {

class ImplBlock;
class FunctionType;
class Field;
class Type;
class ModuleInfo;
struct EncodedSizes;

class Function : public NamedRc, public DocBlockMixin {
public:
    using Pointer = Rc<Function>;
    using ConstPointer = Rc<const Function>;

    Function(bmcl::StringView name, FunctionType* type);
    ~Function();

    FieldVec::ConstRange fieldsRange() const;
    const FunctionType* type() const;
    FunctionType* type();
    const Field* fieldAt(std::size_t index) const;

private:
    Rc<FunctionType> _type;
};

class CmdArgument {
public:
    CmdArgument(Field* field, CmdArgPassKind kind = CmdArgPassKind::Default);
    ~CmdArgument();

    const Field* field() const;
    Field* field();

    CmdArgPassKind argPassKind() const;
    void setArgPassKind(CmdArgPassKind kind);

    const Type* type() const;
    Type* type();

    bmcl::StringView name() const;

private:
    Rc<Field> _field;
    CmdArgPassKind _argPassKind;
};

//TODO: move
class Command : public Function {
public:
    using Pointer = Rc<Command>;
    using ConstPointer = Rc<const Command>;
    using ArgVec = std::vector<CmdArgument>;
    using ArgsRange = IteratorRange<ArgVec::iterator>;
    using ArgsConstRange = IteratorRange<ArgVec::const_iterator>;

    Command(bmcl::StringView name, FunctionType* type);
    ~Command();

    std::uintmax_t number() const;
    void setNumber(std::uintmax_t num);

    ArgsRange argumentsRange();
    ArgsConstRange argumentsRange() const;

    EncodedSizes encodedSizes() const;

private:
    ArgVec _args;
    std::uintmax_t _number;
};
}
