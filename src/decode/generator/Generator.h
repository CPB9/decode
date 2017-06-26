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
#include <unordered_map>

namespace decode {

class Ast;
class Diagnostics;
class Package;
class Project;
class HeaderGen;
class SourceGen;
class NamedType;
class SliceType;
class TypeReprGen;

class Generator : public RefCountable {
public:
    Generator(Diagnostics* diag);

    void setOutPath(bmcl::StringView path);

    bool generateProject(const Project* project);

private:
    bool generateTypesAndComponents(const Ast* ast);
    bool generateSlices();
    bool generateStatusMessages(const Package* package);
    bool generateCommands(const Package* package);
    bool generateTmPrivate(const Package* package);
    bool generateSerializedPackage(const Project* project);
    bool generateDeviceFiles(const Project* project);
    bool generateConfig(const Project* project);
    bool generateInit(const Project* project);

    bool makeDirectory(const char* path);

    void appendModIfdef(bmcl::StringView name);
    void appendEndif();

    bool saveOutput(const char* path, SrcBuilder* output);
    bool dumpIfNotEmpty(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);
    bool dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);

    void appendBuiltinSources(bmcl::StringView ext);

    SrcBuilder _photonPath;
    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    Rc<const Ast> _currentAst;
    std::unique_ptr<HeaderGen> _hgen;
    std::unique_ptr<SourceGen> _sgen;
    Rc<TypeReprGen> _reprGen;
    RcSecondUnorderedMap<std::string, const SliceType> _slices;
};
}
