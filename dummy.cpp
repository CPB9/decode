#include "decode/parser/Lexer.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <assert.h>

int main()
{
    auto rv = bmcl::readFileIntoString("../example.decode");
    decode::parser::Lexer lexer(bmcl::StringView(rv.unwrap()));
}
