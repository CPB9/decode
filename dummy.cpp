#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Package.h"
#include "decode/generator/Generator.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>

#include <iostream>
#include <chrono>
#include <assert.h>

using namespace decode;

void testModel()
{
    auto start = std::chrono::steady_clock::now();

    Rc<Diagnostics> diag = new Diagnostics;
    PackageResult package = Package::readFromDirectory(diag, "../onboard");
    diag->printReports(&std::cout);

    bmcl::Buffer b = package.unwrap()->encode();
    BMCL_DEBUG() << "size:" << b.size();

    PackageResult package2 = Package::decodeFromMemory(diag, b.start(), b.size());
    BMCL_DEBUG() << "decode status:" << package2.isOk();

    b = package2.unwrap()->encode();
    BMCL_DEBUG() << "size:" << b.size();

//     Rc<Generator> gen = makeRc<Generator>(diag);
//     gen->setOutPath("./");
//     bool genOk = gen->generateFromModel(rv.unwrap());
//     if (genOk) {
//         BMCL_DEBUG() << "generating complete";
//     }

    auto end = std::chrono::steady_clock::now();
    BMCL_DEBUG() << "time (us):" << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
}


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
    //process("../onboard/core.decode");
    //process("../onboard/tm.decode");
    testModel();
}
