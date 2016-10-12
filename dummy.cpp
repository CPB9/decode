#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <assert.h>

int main()
{
    decode::parser::Parser p;
    auto rv = p.parseFile("../example.decode");
    if (rv.isOk()) {
        BMCL_DEBUG() << "ok";
    } else {
        BMCL_DEBUG() << "nok";
    }
}
