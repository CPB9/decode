#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Component.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Package.h"
#include "decode/generator/Generator.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>

#include <tclap/CmdLine.h>

#include <iostream>
#include <chrono>
#include <assert.h>

using namespace decode;

void generateSource(const Rc<Package>& package, const char* outPath)
{
    BMCL_DEBUG() << "generating";
    auto start = std::chrono::steady_clock::now();
    //bmcl::Buffer b = package->encode();
    //BMCL_DEBUG() << "size:" << b.size();

    Rc<Generator> gen = makeRc<Generator>(package->diagnostics());
    gen->setOutPath(outPath);
    bool genOk = gen->generateFromPackage(package);
    auto end = std::chrono::steady_clock::now();
    auto delta = end - start;
    if (genOk) {
        BMCL_DEBUG() << "generating complete";
    } else {
        BMCL_DEBUG() << "generating failed";
    }
    BMCL_DEBUG() << "time (us):" << std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
}

int main(int argc, char* argv[])
{
    TCLAP::CmdLine cmdLine("Decode source generator");
    TCLAP::ValueArg<std::string> inPathArg("i", "in", "Input directory", true, "./", "path");
    TCLAP::ValueArg<std::string> outPathArg("o", "out", "Output directory", true, "./", "path");

    cmdLine.add(&inPathArg);
    cmdLine.add(&outPathArg);
    cmdLine.parse(argc, argv);

    Rc<Diagnostics> diag = new Diagnostics;
    PackageResult package = Package::readFromDirectory(diag, inPathArg.getValue().c_str());

    generateSource(package.unwrap(), outPathArg.getValue().c_str());

    diag->printReports(&std::cout);

}
