#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Field.h"

#include <bmcl/Either.h>

#include <unordered_map>

namespace decode {

class ImplBlock;

class Function : public NamedRc {
public:
    Function(bmcl::StringView name, FunctionType* type)
        : NamedRc(name)
        , _type(type)
    {
    }

    const FunctionType* type() const
    {
        return _type.get();
    }

private:
    Rc<FunctionType> _type;
};

class Commands: public RefCountable {
public:
    const std::vector<Rc<Function>>& functions() const
    {
        return _functions;
    }

    void addFunction(Function* func)
    {
        _functions.emplace_back(func);
    }

private:
    std::vector<Rc<Function>> _functions;
};

enum class AccessorKind {
    Field,
    Subscript,
};

class Accessor : public RefCountable {
public:
    AccessorKind accessorKind() const
    {
        return _accessorKind;
    }

protected:
    Accessor(AccessorKind kind)
        : _accessorKind(kind)
    {
    }

private:
    AccessorKind _accessorKind;
};

struct Range {
    bmcl::Option<uintmax_t> lowerBound;
    bmcl::Option<uintmax_t> upperBound;
};

class FieldAccessor : public Accessor {
public:
    FieldAccessor(bmcl::StringView value, Field* field)
        : Accessor(AccessorKind::Field)
        , _value(value)
        , _field(field)
    {
    }

    bmcl::StringView value() const
    {
        return _value;
    }

    const Rc<Field>& field() const
    {
        return _field;
    }

    void setField(Field* field)
    {
        _field.reset(field);
    }

private:
    bmcl::StringView _value;
    Rc<Field> _field;
};

class SubscriptAccessor : public Accessor {
public:
    SubscriptAccessor(const Range& range, Type* type)
        : Accessor(AccessorKind::Subscript)
        , _subscript(range)
        , _type(type)
    {
    }

    SubscriptAccessor(std::uintmax_t subscript, Type* type)
        : Accessor(AccessorKind::Subscript)
        , _subscript(subscript)
        , _type(type)
    {
    }

    const Rc<Type>& type() const
    {
        return _type;
    }

    const bmcl::Either<Range, std::uintmax_t>& subscript() const
    {
        return _subscript;
    }

    void setType(Type* type)
    {
        _type.reset(type);
    }

private:
    bmcl::Either<Range, std::uintmax_t> _subscript;
    Rc<Type> _type;
};

class StatusRegexp : public RefCountable {
public:
    const std::vector<Rc<Accessor>>& accessors() const
    {
        return _accessors;
    }

    std::vector<Rc<Accessor>>& accessors()
    {
        return _accessors;
    }

    void addAccessor(Accessor* acc)
    {
        _accessors.emplace_back(acc);
    }

private:
    std::vector<Rc<Accessor>> _accessors;
};

class StatusMsg : public RefCountable {
public:
    StatusMsg(std::size_t num)
        : _number(num)
    {
    }

    std::size_t number() const
    {
        return _number;
    }

    const std::vector<Rc<StatusRegexp>>& parts() const
    {
        return _parts;
    }

    std::vector<Rc<StatusRegexp>>& parts()
    {
        return _parts;
    }

    void addPart(StatusRegexp* part)
    {
        _parts.emplace_back(part);
    }

private:
    std::vector<Rc<StatusRegexp>> _parts;
    std::size_t _number;
};

typedef std::unordered_map<std::size_t, Rc<StatusMsg>> StatusMap;

class Statuses: public RefCountable {
public:
    const StatusMap& statusMap() const
    {
        return _statusMap;
    }

    StatusMap& statusMap()
    {
        return _statusMap;
    }

    bool addStatus(std::uintmax_t n, StatusMsg* msg)
    {
        auto it = _statusMap.emplace(std::piecewise_construct, std::forward_as_tuple(n), std::forward_as_tuple(msg));
        return it.second;
    }

private:
    StatusMap _statusMap;
};

class Component : public RefCountable {
public:
    Component(std::size_t compNum, const ModuleInfo* info)
        : _number(compNum)
        , _modInfo(info)
    {
    }

    const bmcl::Option<Rc<FieldList>>& parameters() const
    {
        return _params;
    }

    const bmcl::Option<Rc<Commands>>& commands() const
    {
        return _cmds;
    }

    const bmcl::Option<Rc<Statuses>>& statuses() const
    {
        return _statuses;
    }

    const bmcl::Option<Rc<ImplBlock>>& implBlock() const
    {
        return _implBlock;
    }

    bmcl::StringView moduleName() const
    {
        return _modInfo->moduleName();
    }

    std::size_t number() const
    {
        return _number;
    }

    void setParameters(FieldList* params)
    {
        _params.emplace(params);
    }

    void setStatuses(Statuses* statuses)
    {
        _statuses.emplace(statuses);
    }

    void setCommands(Commands* cmds)
    {
        _cmds.emplace(cmds);
    }

    void setImplBlock(ImplBlock* block)
    {
        _implBlock.emplace(block);
    }

    void setNumber(std::size_t number)
    {
        _number = number;
    }

private:
    bmcl::Option<Rc<FieldList>> _params;
    bmcl::Option<Rc<Commands>> _cmds;
    bmcl::Option<Rc<Statuses>> _statuses;
    bmcl::Option<Rc<ImplBlock>> _implBlock;
    std::size_t _number;
    Rc<const ModuleInfo> _modInfo;
};

struct ComponentAndMsg {
    ComponentAndMsg(const Rc<Component>& component, const Rc<StatusMsg>& msg);
    ~ComponentAndMsg();

    Rc<Component> component;
    Rc<StatusMsg> msg;
};
}

