/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Diagnostics.h"
#include "decode/core/Configuration.h"
#include "decode/core/ProgressPrinter.h"
#include "decode/parser/Project.h"
#include "decode/generator/Generator.h"

#include <bmcl/Result.h>

#include <tclap/CmdLine.h>

#include <chrono>

using namespace decode;

int main(int argc, char* argv[])
{
    TCLAP::CmdLine cmdLine("Decode source generator");
    TCLAP::ValueArg<std::string> inPathArg("p", "in", "Project file", true, "./project.toml", "path");
    TCLAP::ValueArg<std::string> outPathArg("o", "out", "Output directory", true, "./", "path");
    TCLAP::ValueArg<unsigned> debugLevelArg("d", "debug-level", "Generated code debug level", false, 0, "0-5");
    TCLAP::SwitchArg verbLevelArg("v", "verbose", "Enable verbose output", false);
    TCLAP::ValueArg<unsigned> compLevelArg("c", "compression-level", "Package compression level", false, 4, "0-5");
    TCLAP::SwitchArg absArg("a", "abs-path", "Use absolute paths for bundled src", false);

    cmdLine.add(&inPathArg);
    cmdLine.add(&outPathArg);
    cmdLine.add(&debugLevelArg);
    cmdLine.add(&verbLevelArg);
    cmdLine.add(&compLevelArg);
    cmdLine.add(&absArg);
    cmdLine.parse(argc, argv);

    auto start = std::chrono::steady_clock::now();
    Rc<Configuration> cfg = new Configuration;

    unsigned debugLevel = std::min(5u, debugLevelArg.getValue());
    cfg->setGeneratedCodeDebugLevel(debugLevel);
    unsigned compLevel = std::min(5u, compLevelArg.getValue());
    cfg->setCompressionLevel(compLevel);
    cfg->setVerboseOutput(verbLevelArg.getValue());

    Rc<Diagnostics> diag = new Diagnostics;
    ProjectResult proj = Project::fromFile(cfg.get(), diag.get(), inPathArg.getValue().c_str());

    if (proj.isErr()) {
        diag->printReports(&std::cerr);
        return -1;
    }

    GeneratorConfig genCfg;
    genCfg.useAbsolutePathsForBundledSources = absArg.getValue();
    proj.unwrap()->generate(outPathArg.getValue().c_str(), genCfg);

    auto end = std::chrono::steady_clock::now();
    auto delta = end - start;
    ProgressPrinter printer(cfg->verboseOutput());
    double microseconds = std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
    printer.printActionProgress("Finished", "in " + std::to_string(microseconds / 1000000) + "s");

    diag->printReports(&std::cout);
}
