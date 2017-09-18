#include "decode/ast/CmdTrait.h"
#include "decode/ast/Function.h"

namespace decode {

CmdTrait::CmdTrait(bmcl::StringView name)
    : NamedRc(name)
{
}

CmdTrait::~CmdTrait()
{
}

RcVec<Function>::Range CmdTrait::functions()
{
    return _functions;
}

RcVec<Function>::ConstRange CmdTrait::functions() const
{
    return _functions;
}

bool CmdTrait::addFunction(Function* func)
{
    _functions.emplace_back(func);
    //TODO: check conflicts
    return true;
}
}
