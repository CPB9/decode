#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

#include <vector>

namespace decode {

class DocBlock : public RefCountable {
public:
    using Pointer = Rc<DocBlock>;
    using ConstPointer = Rc<const DocBlock>;
    using DocVec = std::vector<bmcl::StringView>;
    using DocRange = IteratorRange<DocVec::const_iterator>;

    DocBlock(const DocVec& comments);
    ~DocBlock();

    bmcl::StringView shortDescription() const;
    DocRange longDescription() const;

private:
    bmcl::StringView processComment(bmcl::StringView comment);

    bmcl::StringView _shortDesc;
    std::vector<bmcl::StringView> _longDesc;
};
}
