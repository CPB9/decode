#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"
#include "decode/parser/Token.h"

#include <bmcl/StringView.h>

#include <vector>

namespace decode {
namespace parser {

class Lexer : public RefCountable {
public:
    Lexer();
    Lexer(bmcl::StringView data);

    void reset(bmcl::StringView data);

    void consumeNextToken(Token* dest);
    void peekNextToken(Token* dest);

private:
    std::vector<Token> _tokens;
    bmcl::StringView _data;
    std::size_t _nextToken;
};

}
}
