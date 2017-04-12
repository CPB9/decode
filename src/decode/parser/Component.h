#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Field.h"
#include "decode/parser/Decl.h"

#include <bmcl/Either.h>
#include <bmcl/OptionPtr.h>

#include <unordered_map>

namespace decode {

class ImplBlock;
class FunctionType;

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

enum class AccessorKind {
    Field,
    Subscript,
};

class FieldAccessor;
class SubscriptAccessor;

class Accessor : public RefCountable {
public:
    AccessorKind accessorKind() const
    {
        return _accessorKind;
    }

    FieldAccessor* asFieldAccessor();
    SubscriptAccessor* asSubscriptAccessor();

    const FieldAccessor* asFieldAccessor() const;
    const SubscriptAccessor* asSubscriptAccessor() const;

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

    const Field* field() const
    {
        return _field.get();
    }

    Field* field()
    {
        return _field.get();
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

    const Type* type() const
    {
        return _type.get();
    }

    Type* type()
    {
        return _type.get();
    }

    bool isRange() const
    {
        return _subscript.isFirst();
    }

    bool isIndex() const
    {
        return _subscript.isSecond();
    }

    const Range& asRange() const
    {
        return _subscript.unwrapFirst();
    }

    std::uintmax_t asIndex() const
    {
        return _subscript.unwrapSecond();
    }

    void setType(Type* type)
    {
        _type.reset(type);
    }

private:
    bmcl::Either<Range, std::uintmax_t> _subscript;
    Rc<Type> _type;
};

inline FieldAccessor* Accessor::asFieldAccessor()
{
    return static_cast<FieldAccessor*>(this);
}

inline SubscriptAccessor* Accessor::asSubscriptAccessor()
{
    return static_cast<SubscriptAccessor*>(this);
}

inline const FieldAccessor* Accessor::asFieldAccessor() const
{
    return static_cast<const FieldAccessor*>(this);
}

inline const SubscriptAccessor* Accessor::asSubscriptAccessor() const
{
    return static_cast<const SubscriptAccessor*>(this);
}

class StatusRegexp : public RefCountable {
public:
    using Accessors = RcVec<Accessor>;

    Accessors::Iterator accessorsBegin()
    {
        return _accessors.begin();
    }

    Accessors::Iterator accessorsEnd()
    {
        return _accessors.end();
    }

    Accessors::Range accessorsRange()
    {
        return _accessors;
    }

    Accessors::ConstIterator accessorsBegin() const
    {
        return _accessors.cbegin();
    }

    Accessors::ConstIterator accessorsEnd() const
    {
        return _accessors.cend();
    }

    Accessors::ConstRange accessorsRange() const
    {
        return _accessors;
    }

    bool hasAccessors() const
    {
        return !_accessors.empty();
    }

    void addAccessor(Accessor* acc)
    {
        _accessors.emplace_back(acc);
    }

private:
    Accessors _accessors;
};

class StatusMsg : public RefCountable {
public:
    using Parts = RcVec<StatusRegexp>;

    StatusMsg(std::size_t num)
        : _number(num)
    {
    }

    Parts::Iterator partsBegin()
    {
        return _parts.begin();
    }

    Parts::Iterator partsEnd()
    {
        return _parts.end();
    }

    Parts::Range partsRange()
    {
        return _parts;
    }

    Parts::ConstIterator partsBegin() const
    {
        return _parts.cbegin();
    }

    Parts::ConstIterator partsEnd() const
    {
        return _parts.cend();
    }

    Parts::ConstRange partsRange() const
    {
        return _parts;
    }

    std::size_t number() const
    {
        return _number;
    }

    void addPart(StatusRegexp* part)
    {
        _parts.emplace_back(part);
    }

private:
    Parts _parts;
    std::size_t _number;
};

class Component : public RefCountable {
public:
    using Cmds = RcVec<Function>;
    using Params = FieldVec;
    using Statuses = RcSecondUnorderedMap<std::size_t, StatusMsg>;

    Component(std::size_t compNum, const ModuleInfo* info)
        : _number(compNum)
        , _modInfo(info)
    {
    }

    bool hasParams() const
    {
        return !_params.empty();
    }

    bool hasCmds() const
    {
        return !_cmds.empty();
    }

    bool hasStatuses() const
    {
        return !_statuses.empty();
    }

    Cmds::ConstIterator cmdsBegin() const
    {
        return _cmds.cbegin();
    }

    Cmds::ConstIterator cmdsEnd() const
    {
        return _cmds.cend();
    }

    Cmds::ConstRange cmdsRange() const
    {
        return _cmds;
    }

    Cmds::Iterator cmdsBegin()
    {
        return _cmds.begin();
    }

    Cmds::Iterator cmdsEnd()
    {
        return _cmds.end();
    }

    Cmds::Range cmdsRange()
    {
        return _cmds;
    }

    Params::ConstIterator paramsBegin() const
    {
        return _params.cbegin();
    }

    Params::ConstIterator paramsEnd() const
    {
        return _params.cend();
    }

    Params::ConstRange paramsRange() const
    {
        return _params;
    }

    Params::Iterator paramsBegin()
    {
        return _params.begin();
    }

    Params::Iterator paramsEnd()
    {
        return _params.end();
    }

    Params::Range paramsRange()
    {
        return _params;
    }

    Statuses::ConstIterator statusesBegin() const
    {
        return _statuses.cbegin();
    }

    Statuses::ConstIterator statusesEnd() const
    {
        return _statuses.cend();
    }

    Statuses::ConstRange statusesRange() const
    {
        return _statuses;
    }

    Statuses::Iterator statusesBegin()
    {
        return _statuses.begin();
    }

    Statuses::Iterator statusesEnd()
    {
        return _statuses.end();
    }

    Statuses::Range statusesRange()
    {
        return _statuses;
    }

    bmcl::OptionPtr<const ImplBlock> implBlock() const
    {
        return _implBlock.get();
    }

    bmcl::StringView moduleName() const
    {
        return _modInfo->moduleName();
    }

    bmcl::StringView name() const
    {
        return _modInfo->moduleName();
    }

    std::size_t number() const
    {
        return _number;
    }

    bmcl::OptionPtr<Field> paramWithName(bmcl::StringView name)
    {
        return _params.fieldWithName(name);
    }

    void addParam(Field* param)
    {
        _params.emplace_back(param);
    }

    void addCommand(Function* func)
    {
        _cmds.emplace_back(func);
    }

    bool addStatus(std::uintmax_t n, StatusMsg* msg)
    {
        auto it = _statuses.emplace(std::piecewise_construct, std::forward_as_tuple(n), std::forward_as_tuple(msg));
        return it.second;
    }

    void setImplBlock(ImplBlock* block)
    {
        _implBlock.reset(block);
    }

    void setNumber(std::size_t number)
    {
        _number = number;
    }

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

