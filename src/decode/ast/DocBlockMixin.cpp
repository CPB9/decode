/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/DocBlockMixin.h"
#include "decode/ast/DocBlock.h"

#include <bmcl/OptionPtr.h>

namespace decode {

DocBlockMixin::DocBlockMixin()
{
}

DocBlockMixin::~DocBlockMixin()
{
}

bmcl::OptionPtr<const DocBlock> DocBlockMixin::docs() const
{
    return _docs.get();
}

void DocBlockMixin::setDocs(bmcl::OptionPtr<const DocBlock> docs)
{
    _docs = docs.data();
}

bmcl::StringView DocBlockMixin::shortDescription() const
{
    if (!_docs.isNull()) {
        return _docs->shortDescription();
    }
    return bmcl::StringView::empty();
}
}
