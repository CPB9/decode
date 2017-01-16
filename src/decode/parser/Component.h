#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Either.h>

#include <unordered_map>

namespace decode {

class Parameters: public RefCountable {
public:

    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    Parameters()
        : _fields(new FieldList)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class Commands: public RefCountable {
public:

    const std::vector<Rc<FunctionType>>& functions() const
    {
        return _functions;
    }

protected:
    Commands() = default;

private:
    friend class Parser;

    std::vector<Rc<FunctionType>> _functions;
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
    friend class Parser;

    AccessorKind _accessorKind;
};

struct Range {
    bmcl::Option<uintmax_t> lowerBound;
    bmcl::Option<uintmax_t> upperBound;
};

class FieldAccessor : public Accessor {
public:

    bmcl::StringView value() const
    {
        return _value;
    }

    const Rc<Field>& field() const
    {
        return _field;
    }

protected:
    FieldAccessor()
        : Accessor(AccessorKind::Field)
    {
    }

private:
    friend class Parser;
    friend class Package;

    bmcl::StringView _value;
    Rc<Field> _field;
};

class SubscriptAccessor : public Accessor {
public:

    const Rc<Type>& type() const
    {
        return _type;
    }

    const bmcl::Either<Range, uintmax_t>& subscript() const
    {
        return _subscript;
    }

protected:
    SubscriptAccessor()
        : Accessor(AccessorKind::Subscript)
        , _subscript(bmcl::InPlaceSecond)
    {
    }

private:
    friend class Parser;
    friend class Package;

    Rc<Type> _type;
    bmcl::Either<Range, uintmax_t> _subscript;
};

class StatusRegexp : public RefCountable {
public:
    const std::vector<Rc<Accessor>>& accessors() const
    {
        return _accessors;
    }

protected:
    StatusRegexp() = default;

private:
    friend class Parser;
    friend class Package;

    std::vector<Rc<Accessor>> _accessors;
};

class StatusMsg : public RefCountable {
public:
    std::size_t number() const
    {
        return _number;
    }

    const std::vector<Rc<StatusRegexp>>& parts() const
    {
        return _parts;
    }

protected:
    StatusMsg(std::size_t num)
        : _number(num)
    {
    }

private:
    friend class Parser;
    friend class Package;

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

protected:
    Statuses() = default;


private:
    friend class Parser;
    friend class Package;

    StatusMap _statusMap;
};

class Component : public RefCountable {
public:

    const bmcl::Option<Rc<Parameters>>& parameters() const
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

    bmcl::StringView moduleName() const
    {
        return _moduleName;
    }

    std::size_t number() const
    {
        return _number;
    }

protected:
    Component() = default;

private:
    friend class Parser;
    friend class Package;

    bmcl::Option<Rc<Parameters>> _params;
    bmcl::Option<Rc<Commands>> _cmds;
    bmcl::Option<Rc<Statuses>> _statuses;
    bmcl::StringView _moduleName;
    std::size_t _number;
};

struct ComponentAndMsg {
    ComponentAndMsg(const Rc<Component>& component, const Rc<StatusMsg>& msg);
    ~ComponentAndMsg();

    Rc<Component> component;
    Rc<StatusMsg> msg;
};
}

