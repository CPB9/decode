#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"

#include <bmcl/StringView.h>
#include <bmcl/OptionPtr.h>

#include <vector>

namespace decode {

class DocBlock : public RefCountable {
public:
    using DocVec = std::vector<bmcl::StringView>;
    using DocRange = IteratorRange<DocVec::const_iterator>;

    DocBlock(const DocVec& comments)
    {
        if (comments.empty()) {
            return;
        }
        _shortDesc = processComment(comments.front());
        _longDesc.reserve(comments.size() - 1);
        for (const bmcl::StringView& comment : comments) {
            _longDesc.push_back(processComment(comment));
        }
    }

    bmcl::StringView shortDescription() const
    {
        return _shortDesc;
    }

    DocRange longDescription() const
    {
        return _longDesc;
    }

private:
    bmcl::StringView processComment(bmcl::StringView comment)
    {
        if (comment.size() < 3) {
            return comment.trim();
        }
        return comment.sliceFrom(3).trim();
    }

    bmcl::StringView _shortDesc;
    std::vector<bmcl::StringView> _longDesc;
};

class DocBlockMixin {
public:
    bmcl::OptionPtr<const DocBlock> docs() const
    {
        return _docs.get();
    }

    void setDocs(const DocBlock* docs)
    {
        _docs = docs;
    }

    bmcl::StringView shortDescription() const
    {
        if (_docs) {
            return _docs->shortDescription();
        }
        return bmcl::StringView::empty();
    }

private:
    Rc<const DocBlock> _docs;
};
}
