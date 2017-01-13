#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

#include <cstdint>

namespace decode {

class Type;

class Constant : public RefCountable {
public:

    bmcl::StringView name() const
    {
        return _name;
    }

    std::uintmax_t value() const
    {
        return _value;
    }

    const Rc<Type>& type() const
    {
        return _type;
    }

private:
    friend class Parser;

    bmcl::StringView _name;
    std::uintmax_t _value;
    Rc<Type> _type;
};

}
