#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

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

class Range {
private:
    friend class Parser;

    bmcl::Option<uintmax_t> _lowerBound;
    bmcl::Option<uintmax_t> _upperBound;
};

class FieldAccessor : public Accessor {
public:

    bmcl::StringView value() const
    {
        return _value;
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
protected:
    StatusRegexp() = default;

private:
    friend class Parser;
    friend class Package;

    std::vector<Rc<Accessor>> _accessors;
};

typedef std::unordered_map<std::size_t, std::vector<Rc<StatusRegexp>>> StatusMap;

class Statuses: public RefCountable {
public:
    const StatusMap& statusMap() const
    {
        return _regexps;
    }


protected:
    Statuses() = default;


private:
    friend class Parser;
    friend class Package;

    StatusMap _regexps;
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

protected:
    Component() = default;

private:
    friend class Parser;

    bmcl::Option<Rc<Parameters>> _params;
    bmcl::Option<Rc<Commands>> _cmds;
    bmcl::Option<Rc<Statuses>> _statuses;
    bmcl::StringView _moduleName;
};


}

