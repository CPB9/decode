#pragma once

#include "decode/Config.h"
#include "decode/parser/Decl.h"
#include "decode/parser/AstConsumer.h"

#include <bmcl/StringView.h>
#include <bmcl/ResultFwd.h>

#include <vector>

namespace decode {
namespace parser {

class Lexer;

class ParseError {
public:
private:
};

class DECODE_EXPORT Parser {
public:
    Parser();
    ~Parser();

    bool parse();

private:
    Rc<Lexer> _lexer;
};
}
}
