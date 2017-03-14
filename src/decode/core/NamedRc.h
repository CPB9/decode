#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

namespace decode {

class NamedRc : public RefCountable {
public:
    NamedRc(bmcl::StringView name)
        : _name(name)
    {
    }

    bmcl::StringView name() const
    {
        return _name;
    }

private:
    bmcl::StringView _name;
};

}
