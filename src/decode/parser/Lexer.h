/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/Token.h"

#include <bmcl/StringView.h>

#include <vector>

namespace decode {

class Lexer : public RefCountable {
public:
    Lexer();
    Lexer(bmcl::StringView data);

    void reset(bmcl::StringView data);

    void consumeNextToken(Token* dest);
    void peekNextToken(Token* dest);

    bool nextIs(TokenKind kind);

private:
    std::vector<Token> _tokens;
    bmcl::StringView _data;
    std::size_t _nextToken;
};

}
