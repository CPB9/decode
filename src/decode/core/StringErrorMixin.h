#pragma once

#include "decode/Config.h"

#include <bmcl/Option.h>
#include <bmcl/StringView.h>

#include <string>

namespace decode {

class StringErrorMixin {
public:
    void setErrorMsg(bmcl::StringView error)
    {
        _error.emplace(error.begin(), error.end());
    }

    void resetErrorMsg()
    {
        _error.clear();
    }

    const bmcl::Option<std::string>& errorMsg() const
    {
        return _error;
    }

    bmcl::StringView errorMsgOr(bmcl::StringView msg) const
    {
        if (_error.isSome()) {
            return _error.unwrap();
        }
        return msg;
    }

private:
    bmcl::Option<std::string> _error;
};

}
