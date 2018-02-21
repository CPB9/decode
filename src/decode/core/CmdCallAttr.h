#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"
#include "decode/core/CmdArgPassKind.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

namespace decode {

class CmdCallAttr : public RefCountable {
public:
    using Pointer = Rc<CmdCallAttr>;
    using ConstPointer = Rc<const CmdCallAttr>;

    struct CmdParamPass {
        CmdParamPass(bmcl::StringView name, CmdArgPassKind kind)
            : name(name)
            , kind(kind)
        {
        }

        bmcl::StringView name;
        CmdArgPassKind kind;
    };

    using ParamVec = std::vector<CmdParamPass>;
    using ParamVecConstRange = IteratorRange<ParamVec::const_iterator>;

    CmdCallAttr();
    ~CmdCallAttr();

    void addParam(bmcl::StringView name, CmdArgPassKind kind);

    CmdArgPassKind findArgPassKind(bmcl::StringView name) const;

    ParamVecConstRange params() const;

private:
    ParamVec _params;
};
}
