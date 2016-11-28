#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/generator/Generator.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <iostream>
#include <assert.h>

using namespace decode;

int main()
{
    Rc<Diagnostics> diag = new Diagnostics;
    Parser p(diag);
    auto rv = p.parseFile("../example.decode");
    if (rv.isErr()) {
        BMCL_CRITICAL() << "errors found: " << p.currentLoc().line << ":" << p.currentLoc().column;
        diag->printReports(&std::cout);
        return 1;
    } else {
        BMCL_DEBUG() << "parsing complete";
        diag->printReports(&std::cout);
    }

    Rc<Generator> gen = makeRc<Generator>(diag);
    gen->setOutPath("./");
    bool genOk = gen->generateFromAst(rv.unwrap());
    if (genOk) {
        BMCL_DEBUG() << "generating complete";
    }
}
