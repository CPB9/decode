#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"
#include "decode/generator/Generator.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <iostream>
#include <chrono>
#include <assert.h>

using namespace decode;

void process(const char* path)
{
    auto start = std::chrono::steady_clock::now();


    Rc<Diagnostics> diag = new Diagnostics;
    Parser p(diag);
    auto rv = p.parseFile(path);
    if (rv.isErr()) {
        BMCL_CRITICAL() << "errors found: " << p.currentLoc().line << ":" << p.currentLoc().column;
        diag->printReports(&std::cout);
        return;
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

    auto end = std::chrono::steady_clock::now();
    BMCL_DEBUG() << "time (us):" << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
}

int main()
{
    process("../test.decode");
    process("../onboard.decode");
}
