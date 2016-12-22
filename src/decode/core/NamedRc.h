#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

namespace decode {

class NamedRc : public RefCountable {
public:
    bmcl::StringView name() const
    {
        return _name;
    }

private:
    friend class Parser;

    bmcl::StringView _name;
};

}
