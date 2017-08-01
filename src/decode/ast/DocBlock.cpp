/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/DocBlock.h"

#include <bmcl/OptionPtr.h>

namespace decode {

DocBlock::DocBlock(const DocVec& comments)
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

DocBlock::~DocBlock()
{
}

bmcl::StringView DocBlock::shortDescription() const
{
    return _shortDesc;
}

DocBlock::DocRange DocBlock::longDescription() const
{
    return _longDesc;
}

bmcl::StringView DocBlock::processComment(bmcl::StringView comment)
{
    if (comment.size() < 3) {
        return comment.trim();
    }
    return comment.sliceFrom(3).trim();
}
}
