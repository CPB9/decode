/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/parser/Containers.h"

#include <bmcl/StringView.h>

#include <memory>

namespace decode {

class Ast;
class Diagnostics;
class Package;
class Project;
class OnboardTypeHeaderGen;
class OnboardTypeSourceGen;
class NamedType;
class DynArrayType;
class TypeReprGen;

struct GeneratorConfig {
    GeneratorConfig()
        : useAbsolutePathsForBundledSources(false)
      //  , generateOnboard(true)
      //  , generateGroundcontrol(true)
    {
    }

    bool useAbsolutePathsForBundledSources;
    //bool generateOnboard;
    //bool generateGroundcontrol;
};

class Generator : public RefCountable {
public:
    using Pointer = Rc<Generator>;
    using ConstPointer = Rc<const Generator>;

    Generator(Diagnostics* diag);
    ~Generator();

    void setOutPath(bmcl::StringView path);

    bool generateProject(const Project* project, const GeneratorConfig& cfg = GeneratorConfig());

private:
    bool generateTypesAndComponents(const Ast* ast);
    bool generateDynArrays(const Package* package);
    bool generateStatusMessages(const Project* package);
    bool generateCommands(const Package* package);
    bool generateTmPrivate(const Package* package);
    bool generateGenerics(const Package* package);
    static void generateSerializedPackage(const Project* project, bmcl::Buffer* serialized, SrcBuilder* sourceCode);
    bool generateDeviceFiles(const Project* project);
    bool generateConfig(const Project* project);

    void appendModIfdef(bmcl::StringView name);
    void appendEndif();

    bool dumpIfNotEmpty(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);
    bool dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);

    void appendBuiltinHeaders();
    void appendBuiltinSources();
    void appendBuiltins(bmcl::ArrayView<bmcl::StringView> names, bmcl::StringView ext);

    Rc<Diagnostics> _diag;
    std::string _savePath;
    std::string _photongenPath;
    SrcBuilder _onboardPath;
    SrcBuilder _gcPath;
    SrcBuilder _output;
    std::unique_ptr<OnboardTypeHeaderGen> _onboardHgen;
    std::unique_ptr<OnboardTypeSourceGen> _onboardSgen;
    GeneratorConfig _config;
};
}
