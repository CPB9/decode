/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Component.h"
#include "decode/core/Foreach.h"
#include "decode/core/StringBuilder.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Field.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"

#include <bmcl/OptionPtr.h>

namespace decode {

Accessor::Accessor(AccessorKind kind)
    : _accessorKind(kind)
{
}

Accessor::~Accessor()
{
}

AccessorKind Accessor::accessorKind() const
{
    return _accessorKind;
}

bool Accessor::isFieldAccessor() const
{
    return _accessorKind == AccessorKind::Field;
}

bool Accessor::isSubscriptAccessor() const
{
    return _accessorKind == AccessorKind::Subscript;
}

FieldAccessor* Accessor::asFieldAccessor()
{
    assert(isFieldAccessor());
    return static_cast<FieldAccessor*>(this);
}

SubscriptAccessor* Accessor::asSubscriptAccessor()
{
    assert(isSubscriptAccessor());
    return static_cast<SubscriptAccessor*>(this);
}

const FieldAccessor* Accessor::asFieldAccessor() const
{
    assert(isFieldAccessor());
    return static_cast<const FieldAccessor*>(this);
}

const SubscriptAccessor* Accessor::asSubscriptAccessor() const
{
    assert(isSubscriptAccessor());
    return static_cast<const SubscriptAccessor*>(this);
}

FieldAccessor::FieldAccessor(bmcl::StringView value, Field* field)
    : Accessor(AccessorKind::Field)
    , _value(value)
    , _field(field)
{
}

FieldAccessor::~FieldAccessor()
{
}

bmcl::StringView FieldAccessor::value() const
{
    return _value;
}

const Field* FieldAccessor::field() const
{
    return _field.get();
}

Field* FieldAccessor::field()
{
    return _field.get();
}

void FieldAccessor::setField(Field* field)
{
    _field.reset(field);
}

SubscriptAccessor::SubscriptAccessor(const Range& range, Type* type)
    : Accessor(AccessorKind::Subscript)
    , _subscript(range)
    , _type(type)
{
}

SubscriptAccessor::SubscriptAccessor(std::uintmax_t subscript, Type* type)
    : Accessor(AccessorKind::Subscript)
    , _subscript(subscript)
    , _type(type)
{
}

SubscriptAccessor::~SubscriptAccessor()
{
}

const Type* SubscriptAccessor::type() const
{
    return _type.get();
}

Type* SubscriptAccessor::type()
{
    return _type.get();
}

bool SubscriptAccessor::isRange() const
{
    return _subscript.isFirst();
}

bool SubscriptAccessor::isIndex() const
{
    return _subscript.isSecond();
}

const Range& SubscriptAccessor::asRange() const
{
    return _subscript.unwrapFirst();
}

std::uintmax_t SubscriptAccessor::asIndex() const
{
    return _subscript.unwrapSecond();
}

void SubscriptAccessor::setType(Type* type)
{
    _type.reset(type);
}

StatusRegexp::StatusRegexp()
    : _type(nullptr)
{
}

StatusRegexp::~StatusRegexp()
{
}

StatusRegexp::Accessors::Iterator StatusRegexp::accessorsBegin()
{
    return _accessors.begin();
}

StatusRegexp::Accessors::Iterator StatusRegexp::accessorsEnd()
{
    return _accessors.end();
}

StatusRegexp::Accessors::Range StatusRegexp::accessorsRange()
{
    return _accessors;
}

StatusRegexp::Accessors::ConstIterator StatusRegexp::accessorsBegin() const
{
    return _accessors.cbegin();
}

StatusRegexp::Accessors::ConstIterator StatusRegexp::accessorsEnd() const
{
    return _accessors.cend();
}

StatusRegexp::Accessors::ConstRange StatusRegexp::accessorsRange() const
{
    return _accessors;
}

bool StatusRegexp::hasAccessors() const
{
    return !_accessors.empty();
}

void StatusRegexp::addAccessor(Accessor* acc)
{
    _accessors.emplace_back(acc);
}

const Type* StatusRegexp::type() const
{
    return _type.get();
}

Type* StatusRegexp::type()
{
    return _type.get();
}

void StatusRegexp::setType(Type* type)
{
    _type = type;
}

void StatusRegexp::buildFieldName(StringBuilder* dest) const
{
    foreachList(accessorsRange(), [dest](const Accessor* acc) {
         switch (acc->accessorKind()) {
            case AccessorKind::Field: {
                bmcl::StringView name = acc->asFieldAccessor()->field()->name();
                dest->append(name.begin(), name.end());
                break;
            }
            case AccessorKind::Subscript: {
                break;
            }
            default:
                assert(false);
        }
    }, [&](const Accessor* acc) {
        dest->append("_");
    });
}

TmMsg::TmMsg(bmcl::StringView name, std::size_t number, bool isEnabled)
    : _name(name)
    , _number(number)
    , _isEnabled(isEnabled)
{
}

TmMsg::~TmMsg()
{
}

bmcl::StringView TmMsg::name() const
{
    return _name;
}

std::size_t TmMsg::number() const
{
    return _number;
}

bool TmMsg::isEnabled() const
{
    return _isEnabled;
}


StatusMsg::StatusMsg(bmcl::StringView name, std::size_t number, std::size_t priority, bool isEnabled)
    : TmMsg(name, number, isEnabled)
    , _priority(priority)
{
}

StatusMsg::~StatusMsg()
{
}

StatusMsg::Parts::Iterator StatusMsg::partsBegin()
{
    return _parts.begin();
}

StatusMsg::Parts::Iterator StatusMsg::partsEnd()
{
    return _parts.end();
}

StatusMsg::Parts::Range StatusMsg::partsRange()
{
    return _parts;
}

StatusMsg::Parts::ConstIterator StatusMsg::partsBegin() const
{
    return _parts.cbegin();
}

StatusMsg::Parts::ConstIterator StatusMsg::partsEnd() const
{
    return _parts.cend();
}

StatusMsg::Parts::ConstRange StatusMsg::partsRange() const
{
    return _parts;
}

std::size_t StatusMsg::priority() const
{
    return _priority;
}


void StatusMsg::addPart(StatusRegexp* part)
{
    _parts.emplace_back(part);
}

EventMsg::EventMsg(bmcl::StringView name, std::size_t number, bool isEnabled)
    : TmMsg(name, number, isEnabled)
{
}

EventMsg::~EventMsg()
{
}

FieldVec::ConstRange EventMsg::partsRange() const
{
    return _fields;
}

void EventMsg::addField(Field* field)
{
    _fields.emplace_back(field);
}

Component::Component(std::size_t compNum, const ModuleInfo* info)
    : _number(compNum)
    , _modInfo(info)
{
}

Component::~Component()
{
}

bool Component::hasParams() const
{
    return !_params.empty();
}

bool Component::hasCmds() const
{
    return !_cmds.empty();
}

bool Component::hasStatuses() const
{
    return !_statuses.empty();
}

bool Component::hasEvents() const
{
    return !_events.empty();
}

Component::Cmds::ConstIterator Component::cmdsBegin() const
{
    return _cmds.cbegin();
}

Component::Cmds::ConstIterator Component::cmdsEnd() const
{
    return _cmds.cend();
}

Component::Cmds::ConstRange Component::cmdsRange() const
{
    return _cmds;
}

Component::Cmds::Iterator Component::cmdsBegin()
{
    return _cmds.begin();
}

Component::Cmds::Iterator Component::cmdsEnd()
{
    return _cmds.end();
}

Component::Cmds::Range Component::cmdsRange()
{
    return _cmds;
}

Component::Params::ConstIterator Component::paramsBegin() const
{
    return _params.cbegin();
}

Component::Params::ConstIterator Component::paramsEnd() const
{
    return _params.cend();
}

Component::Params::ConstRange Component::paramsRange() const
{
    return _params;
}

Component::Params::Iterator Component::paramsBegin()
{
    return _params.begin();
}

Component::Params::Iterator Component::paramsEnd()
{
    return _params.end();
}

Component::Params::Range Component::paramsRange()
{
    return _params;
}

Component::Statuses::ConstIterator Component::statusesBegin() const
{
    return _statuses.cbegin();
}

Component::Statuses::ConstIterator Component::statusesEnd() const
{
    return _statuses.cend();
}

Component::Statuses::ConstRange Component::statusesRange() const
{
    return _statuses;
}

Component::Events::ConstIterator Component::eventsBegin() const
{
    return _events.cbegin();
}

Component::Events::ConstIterator Component::eventsEnd() const
{
    return _events.cend();
}

Component::Events::ConstRange Component::eventsRange() const
{
    return _events;
}

Component::Statuses::Iterator Component::statusesBegin()
{
    return _statuses.begin();
}

Component::Statuses::Iterator Component::statusesEnd()
{
    return _statuses.end();
}

Component::Statuses::Range Component::statusesRange()
{
    return _statuses;
}

bmcl::OptionPtr<const ImplBlock> Component::implBlock() const
{
    return _implBlock.get();
}

bmcl::StringView Component::moduleName() const
{
    return _modInfo->moduleName();
}

const ModuleInfo* Component::moduleInfo() const
{
    return _modInfo.get();
}

bmcl::StringView Component::name() const
{
    return _modInfo->moduleName();
}

std::size_t Component::number() const
{
    return _number;
}

bmcl::OptionPtr<const Field> Component::paramWithName(bmcl::StringView name) const
{
    return _params.fieldWithName(name);
}

bmcl::OptionPtr<const Command> Component::cmdWithName(bmcl::StringView name) const
{
    for (const Rc<Command>& value : _cmds) {
        if (value->name() == name) {
            return value.get();
        }
    }
    return bmcl::None;
}

void Component::addParam(Field* param)
{
    _params.emplace_back(param);
}

void Component::addCommand(Command* func)
{
    _cmds.emplace_back(func);
}

bool Component::addStatus(StatusMsg* msg)
{
    auto it = _statuses.emplace(std::piecewise_construct, std::forward_as_tuple(msg->name()), std::forward_as_tuple(msg));
    return it.second;
}

bool Component::addEvent(EventMsg* msg)
{
    auto it = _events.emplace(std::piecewise_construct, std::forward_as_tuple(msg->name()), std::forward_as_tuple(msg));
    return it.second;
}

void Component::setImplBlock(ImplBlock* block)
{
    _implBlock.reset(block);
}

void Component::setNumber(std::size_t number)
{
    _number = number;
}

ComponentAndMsg::ComponentAndMsg(const Rc<Component>& component, const Rc<StatusMsg>& msg)
    : component(component)
    , msg(msg)
{
}

ComponentAndMsg::~ComponentAndMsg()
{
}
}
