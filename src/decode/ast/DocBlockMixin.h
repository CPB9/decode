#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

namespace decode {

class DocBlock;

class DocBlockMixin {
public:
    DocBlockMixin();
    ~DocBlockMixin();

    bmcl::OptionPtr<const DocBlock> docs() const;
    void setDocs(bmcl::OptionPtr<const DocBlock> docs);
    bmcl::StringView shortDescription() const;

private:
    Rc<const DocBlock> _docs;
};
}
