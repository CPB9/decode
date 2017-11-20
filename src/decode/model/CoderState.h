#pragma once

#include <bmcl/StringView.h>

namespace decode {

class CoderState {
public:
    void setError(bmcl::StringView msg);
};

}
