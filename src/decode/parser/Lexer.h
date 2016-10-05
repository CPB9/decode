#pragma once

#include "decode/Config.h"
#include "decode/parser/Token.h"

#include <bmcl/StringView.h>

#include <vector>

namespace decode {
namespace parser {

class Lexer {
public:

    Lexer(bmcl::StringView data);

    void reset(bmcl::StringView data);

private:
    std::vector<Token> _tokens;
    bmcl::StringView _data;
};

}
}
