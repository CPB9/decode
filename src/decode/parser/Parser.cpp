#include "decode/parser/Parser.h"
#include "decode/parser/Decl.h"
#include "decode/parser/ParseTree.h"
#include "decode/parser/Token.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Lexer.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>

#include <pegtl/input.hh>



namespace decode {
namespace parser {

static inline bmcl::StringView sv(const pegtl::input& in)
{
    return bmcl::StringView(in.begin(), in.end());
}

Parser::~Parser()
{
}

bool Parser::parse()
{
    bmcl::Result<std::string, int> rv = bmcl::readFileIntoString(fname);
    _lexer = new Lexer(bmcl::StringView(rv.unwrap()));

    return true;
}
}
}
