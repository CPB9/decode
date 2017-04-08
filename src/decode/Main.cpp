#include "decode/core/Diagnostics.h"
#include "decode/core/Configuration.h"
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
#include <algorithm>
#include <assert.h>

using namespace decode;

void generateSource(Package* package, const char* outPath)
{
    BMCL_DEBUG() << "generating";
    auto start = std::chrono::steady_clock::now();
    //bmcl::Buffer b = package->encode();
    //BMCL_DEBUG() << "size:" << b.size();

    Rc<Generator> gen = new Generator(package->diagnostics());
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
    TCLAP::ValueArg<unsigned> debugLevelArg("d", "debug-level", "Generated code debug level", false, 0, "0-5");
    TCLAP::ValueArg<unsigned> compLevelArg("c", "compression-level", "Package compression level", false, 5, "0-5");

    cmdLine.add(&inPathArg);
    cmdLine.add(&outPathArg);
    cmdLine.add(&debugLevelArg);
    cmdLine.add(&compLevelArg);
    cmdLine.parse(argc, argv);

    Rc<Configuration> cfg = new Configuration;
    unsigned debugLevel = std::min(5u, debugLevelArg.getValue());
    cfg->setDebugLevel(debugLevel);
    unsigned compLevel = std::min(5u, compLevelArg.getValue());
    cfg->setCompressionLevel(compLevel);

    Rc<Diagnostics> diag = new Diagnostics;
    PackageResult package = Package::readFromDirectory(cfg.get(), diag.get(), inPathArg.getValue().c_str());

    generateSource(package.unwrap().get(), outPathArg.getValue().c_str());

    diag->printReports(&std::cout);

}
