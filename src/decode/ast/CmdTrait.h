#pragma once

#include "decode/Config.h"
#include "decode/core/NamedRc.h"
#include "decode/parser/Containers.h"

namespace decode {

class Function;

class CmdTrait : public NamedRc {
public:
    CmdTrait(bmcl::StringView name);
    ~CmdTrait();

    RcVec<Function>::Range functions();
    RcVec<Function>::ConstRange functions() const;

    bool addFunction(Function* func);

private:
    RcVec<Function> _functions;
};
}
