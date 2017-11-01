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
#include "decode/core/Hash.h"
#include "decode/ast/DocBlockMixin.h"
#include "decode/parser/Containers.h"

#include <bmcl/Fwd.h>
#include <bmcl/Either.h>
#include <bmcl/Option.h>

namespace decode {

class ImplBlock;
class Function;
class Command;
class FunctionType;
class Field;
class Type;
class ModuleInfo;
class FieldAccessor;
class SubscriptAccessor;

enum class AccessorKind {
    Field,
    Subscript,
};

class Accessor : public RefCountable {
public:
    using Pointer = Rc<Accessor>;
    using ConstPointer = Rc<const Accessor>;

    ~Accessor();

    AccessorKind accessorKind() const;

    FieldAccessor* asFieldAccessor();
    SubscriptAccessor* asSubscriptAccessor();

    const FieldAccessor* asFieldAccessor() const;
    const SubscriptAccessor* asSubscriptAccessor() const;

protected:
    Accessor(AccessorKind kind);

private:
    AccessorKind _accessorKind;
};

struct Range {
    bmcl::Option<uintmax_t> lowerBound;
    bmcl::Option<uintmax_t> upperBound;
};

class FieldAccessor : public Accessor {
public:
    using Pointer = Rc<FieldAccessor>;
    using ConstPointer = Rc<const FieldAccessor>;

    FieldAccessor(bmcl::StringView value, Field* field);
    ~FieldAccessor();

    bmcl::StringView value() const;
    const Field* field() const;
    Field* field();

    void setField(Field* field);

private:
    bmcl::StringView _value;
    Rc<Field> _field;
};

class SubscriptAccessor : public Accessor {
public:
    using Pointer = Rc<SubscriptAccessor>;
    using ConstPointer = Rc<const SubscriptAccessor>;

    SubscriptAccessor(const Range& range, Type* type);
    SubscriptAccessor(std::uintmax_t subscript, Type* type);
    ~SubscriptAccessor();

    const Type* type() const;
    Type* type();
    bool isRange() const;
    bool isIndex() const;
    const Range& asRange() const;
    std::uintmax_t asIndex() const;

    void setType(Type* type);

private:
    bmcl::Either<Range, std::uintmax_t> _subscript;
    Rc<Type> _type;
};

class StatusRegexp : public RefCountable {
public:
    using Pointer = Rc<StatusRegexp>;
    using ConstPointer = Rc<const StatusRegexp>;
    using Accessors = RcVec<Accessor>;

    StatusRegexp();
    ~StatusRegexp();

    Accessors::Iterator accessorsBegin();
    Accessors::Iterator accessorsEnd();
    Accessors::Range accessorsRange();
    Accessors::ConstIterator accessorsBegin() const;
    Accessors::ConstIterator accessorsEnd() const;
    Accessors::ConstRange accessorsRange() const;
    bool hasAccessors() const;

    void addAccessor(Accessor* acc);

private:
    Accessors _accessors;
};

class StatusMsg : public RefCountable {
public:
    using Pointer = Rc<StatusMsg>;
    using ConstPointer = Rc<const StatusMsg>;
    using Parts = RcVec<StatusRegexp>;

    StatusMsg(bmcl::StringView name, std::size_t priority, bool isEnabled);
    ~StatusMsg();

    Parts::Iterator partsBegin();
    Parts::Iterator partsEnd();
    Parts::Range partsRange();
    Parts::ConstIterator partsBegin() const;
    Parts::ConstIterator partsEnd() const;
    Parts::ConstRange partsRange() const;
    bmcl::StringView name() const;
    std::size_t number() const;
    std::size_t priority() const;
    bool isEnabled() const;

    void setNumber(std::size_t num);
    void addPart(StatusRegexp* part);

private:
    Parts _parts;
    std::size_t _number;
    std::size_t _priority;
    bmcl::StringView _name;
    bool _isEnabled;
};

class Component : public RefCountable {
public:
    using Pointer = Rc<Component>;
    using ConstPointer = Rc<const Component>;
    using Cmds = RcVec<Command>;
    using Params = FieldVec;
    using Statuses = RcSecondUnorderedMap<bmcl::StringView, StatusMsg>;

    Component(std::size_t compNum, const ModuleInfo* info);
    ~Component();

    bool hasParams() const;
    bool hasCmds() const;
    bool hasStatuses() const;
    Cmds::Iterator cmdsBegin();
    Cmds::Iterator cmdsEnd();
    Cmds::Range cmdsRange();
    Cmds::ConstIterator cmdsBegin() const;
    Cmds::ConstIterator cmdsEnd() const;
    Cmds::ConstRange cmdsRange() const;
    Params::Iterator paramsBegin();
    Params::Iterator paramsEnd();
    Params::Range paramsRange();
    Params::ConstIterator paramsBegin() const;
    Params::ConstIterator paramsEnd() const;
    Params::ConstRange paramsRange() const;
    Statuses::Iterator statusesBegin();
    Statuses::Iterator statusesEnd();
    Statuses::Range statusesRange();
    Statuses::ConstIterator statusesBegin() const;
    Statuses::ConstIterator statusesEnd() const;
    Statuses::ConstRange statusesRange() const;
    bmcl::OptionPtr<const ImplBlock> implBlock() const;
    bmcl::StringView moduleName() const;
    const ModuleInfo* moduleInfo() const;
    bmcl::StringView name() const;
    std::size_t number() const;

    bmcl::OptionPtr<const Field> paramWithName(bmcl::StringView name) const;
    bmcl::OptionPtr<const Command> cmdWithName(bmcl::StringView name) const;

    void addParam(Field* param);
    void addCommand(Command* func);
    bool addStatus(StatusMsg* msg);
    void setImplBlock(ImplBlock* block);
    void setNumber(std::size_t number);

private:
    std::size_t _number;
    Params _params;
    Cmds _cmds;
    Statuses _statuses;
    Rc<ImplBlock> _implBlock;
    Rc<const ModuleInfo> _modInfo;
};

struct ComponentAndMsg {
    ComponentAndMsg(const Rc<Component>& component, const Rc<StatusMsg>& msg);
    ~ComponentAndMsg();

    Rc<Component> component;
    Rc<StatusMsg> msg;
};
}

