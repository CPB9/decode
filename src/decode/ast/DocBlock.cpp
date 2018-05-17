/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/DocBlock.h"

#include <bmcl/OptionPtr.h>

#include <tao/pegtl/internal/peek_utf8.hpp>

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

struct StringInput {
    const char* current;
    const char* end;

    char peek_byte(std::size_t offset = 0) const noexcept
    {
        return current[offset];
    }
    std::size_t size( const std::size_t /*unused*/ = 0 ) const noexcept
    {
       return std::size_t(this->end - this->current);
    }

    bool atEnd() const
    {
        return current == end;
    }
};

static inline bool isSpace(uint32_t c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\v':
    case '\r':
    case '\n':
        return true;
    }
    return false;
}

bmcl::StringView DocBlock::processComment(bmcl::StringView comment)
{
    StringInput input{comment.begin(), comment.end()};
    tao::pegtl::internal::peek_utf8 peek;

    while (true) {
        auto p = peek.peek(input);
        if (isSpace(p.data) || p.data == '/') {
            input.current += p.size;
        } else {
            break;
        }
        if (input.atEnd()) {
            return bmcl::StringView(input.current, input.end);
        }
    }

    const char* commendBegin = input.current;
    const char* rtrimBegin = nullptr;

    while (true) {
        auto p = peek.peek(input);
        if (isSpace(p.data)) {
            rtrimBegin = input.current;
        } else {
            rtrimBegin = nullptr;
        }
        input.current += p.size;
        if (input.atEnd()) {
            break;
        }
    }

    if (rtrimBegin) {
        return bmcl::StringView(commendBegin, rtrimBegin);
    }
    return bmcl::StringView(commendBegin, input.end);
}
}
