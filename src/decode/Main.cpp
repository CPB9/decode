/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Diagnostics.h"
#include "decode/core/Configuration.h"
#include "decode/core/Iterator.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Component.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Package.h"
#include "decode/parser/Project.h"
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

int main(int argc, char* argv[])
{
    TCLAP::CmdLine cmdLine("Decode source generator");
    TCLAP::ValueArg<std::string> inPathArg("p", "in", "Project file", true, "./project.toml", "path");
    TCLAP::ValueArg<std::string> outPathArg("o", "out", "Output directory", true, "./", "path");
    TCLAP::ValueArg<unsigned> debugLevelArg("d", "debug-level", "Generated code debug level", false, 0, "0-5");
    TCLAP::ValueArg<unsigned> compLevelArg("c", "compression-level", "Package compression level", false, 4, "0-5");

    cmdLine.add(&inPathArg);
    cmdLine.add(&outPathArg);
    cmdLine.add(&debugLevelArg);
    cmdLine.add(&compLevelArg);
    cmdLine.parse(argc, argv);

    auto start = std::chrono::steady_clock::now();
    Rc<Configuration> cfg = new Configuration;

    unsigned debugLevel = std::min(5u, debugLevelArg.getValue());
    cfg->setDebugLevel(debugLevel);
    unsigned compLevel = std::min(5u, compLevelArg.getValue());
    cfg->setCompressionLevel(compLevel);

    Rc<Diagnostics> diag = new Diagnostics;
    ProjectResult proj = Project::fromFile(cfg.get(), diag.get(), inPathArg.getValue().c_str());

    if (proj.isErr()) {
        diag->printReports(&std::cout);
        return 0;
    }

    proj.unwrap()->generate(outPathArg.getValue().c_str());

    auto end = std::chrono::steady_clock::now();
    auto delta = end - start;
    BMCL_DEBUG() << "time (us):" << std::chrono::duration_cast<std::chrono::microseconds>(delta).count();

    diag->printReports(&std::cout);
}
