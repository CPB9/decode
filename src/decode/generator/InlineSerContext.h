/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/StringBuilder.h"

#include <cstdint>

namespace decode {

struct InlineSerContext {
    InlineSerContext(std::uint8_t indentLevel = 1, std::uint8_t loopLevel = 0, std::uint8_t ptrDepth = 0)
        : indentLevel(indentLevel)
        , loopLevel(loopLevel)
        , ptrDepth(ptrDepth)
    {
    }

    InlineSerContext indent() const
    {
        return InlineSerContext(indentLevel + 1, loopLevel, ptrDepth);
    }

    InlineSerContext incLoopVar() const
    {
        return InlineSerContext(indentLevel, loopLevel + 1, ptrDepth);
    }

    InlineSerContext incPtrDepth() const
    {
        return InlineSerContext(indentLevel, loopLevel, ptrDepth + 1);
    }

    void appendIndent(StringBuilder* output) const
    {
        output->appendSeveral(indentLevel, "    ");
    }

    char currentLoopVar() const
    {
        return 'a' + loopLevel;
    }

    std::uint8_t indentLevel;
    std::uint8_t loopLevel;
    std::uint8_t ptrDepth;
};
}
