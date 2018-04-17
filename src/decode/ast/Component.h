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
class StringBuilder;

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

    bool isFieldAccessor() const;
    bool isSubscriptAccessor() const;

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

    const Type* type() const;
    Type* type();
    void setType(Type* type);

    void buildFieldName(StringBuilder* dest) const;

private:
    Accessors _accessors;
    Rc<Type> _type;
};

class TmMsg : public RefCountable {
public:
    using Pointer = Rc<TmMsg>;
    using ConstPointer = Rc<const TmMsg>;

    TmMsg(bmcl::StringView name, std::size_t number, bool isEnabled);
    ~TmMsg();

    bmcl::StringView name() const;
    std::size_t number() const;
    bool isEnabled() const;

private:
    bmcl::StringView _name;
    std::size_t _number;
    bool _isEnabled;
};

class StatusMsg : public TmMsg {
public:
    using Pointer = Rc<StatusMsg>;
    using ConstPointer = Rc<const StatusMsg>;
    using Parts = RcVec<StatusRegexp>;

    StatusMsg(bmcl::StringView name, std::size_t number, std::size_t priority, bool isEnabled);
    ~StatusMsg();

    Parts::Iterator partsBegin();
    Parts::Iterator partsEnd();
    Parts::Range partsRange();
    Parts::ConstIterator partsBegin() const;
    Parts::ConstIterator partsEnd() const;
    Parts::ConstRange partsRange() const;
    std::size_t priority() const;

    void addPart(StatusRegexp* part);

private:
    Parts _parts;
    std::size_t _priority;
};

class EventMsg : public TmMsg {
public:
    using Pointer = Rc<EventMsg>;
    using ConstPointer = Rc<const EventMsg>;

    EventMsg(bmcl::StringView name, std::size_t number, bool isEnabled);
    ~EventMsg();

    FieldVec::ConstRange partsRange() const;

    void addField(Field* field); //TODO: check conflicts

private:
    FieldVec _fields;
};

class Component : public RefCountable {
public:
    using Pointer = Rc<Component>;
    using ConstPointer = Rc<const Component>;
    using Cmds = RcVec<Command>;
    using Vars = FieldVec;
    using Statuses = RcSecondUnorderedMap<bmcl::StringView, StatusMsg>;
    using Events = RcSecondUnorderedMap<bmcl::StringView, EventMsg>;

    Component(std::size_t compNum, const ModuleInfo* info);
    ~Component();

    bool hasVars() const;
    bool hasCmds() const;
    bool hasStatuses() const;
    bool hasEvents() const;
    Cmds::Iterator cmdsBegin();
    Cmds::Iterator cmdsEnd();
    Cmds::Range cmdsRange();
    Cmds::ConstIterator cmdsBegin() const;
    Cmds::ConstIterator cmdsEnd() const;
    Cmds::ConstRange cmdsRange() const;
    Vars::Iterator varsBegin();
    Vars::Iterator varsEnd();
    Vars::Range varsRange();
    Vars::ConstIterator varsBegin() const;
    Vars::ConstIterator varsEnd() const;
    Vars::ConstRange varsRange() const;
    Statuses::Iterator statusesBegin();
    Statuses::Iterator statusesEnd();
    Statuses::Range statusesRange();
    Statuses::ConstIterator statusesBegin() const;
    Statuses::ConstIterator statusesEnd() const;
    Statuses::ConstRange statusesRange() const;
    Events::ConstIterator eventsBegin() const;
    Events::ConstIterator eventsEnd() const;
    Events::ConstRange eventsRange() const;
    bmcl::OptionPtr<const ImplBlock> implBlock() const;
    bmcl::StringView moduleName() const;
    const ModuleInfo* moduleInfo() const;
    bmcl::StringView name() const;
    std::size_t number() const;

    bmcl::OptionPtr<const Field> varWithName(bmcl::StringView name) const;
    bmcl::OptionPtr<const Command> cmdWithName(bmcl::StringView name) const;

    void addVar(Field* var); //TODO: check name conflicts
    void addCommand(Command* func); //TODO: check name conflicts
    bool addStatus(StatusMsg* msg);
    bool addEvent(EventMsg* msg);
    void setImplBlock(ImplBlock* block);
    void setNumber(std::size_t number);

private:
    std::size_t _number;
    Vars _vars;
    Cmds _cmds;
    Statuses _statuses;
    Events _events;
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

