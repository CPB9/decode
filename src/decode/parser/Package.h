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
#include "decode/parser/Containers.h"

#include <bmcl/Fwd.h>
#include <bmcl/Buffer.h>

namespace decode {

class Ast;
class Diagnostics;
class Parser;
class Package;
class Component;
class VarRegexp;
class Configuration;
struct ComponentAndMsg;

using PackageResult = bmcl::Result<Rc<Package>, void>;

class Package : public RefCountable {
public:
    using Pointer = Rc<Package>;
    using ConstPointer = Rc<const Package>;

    struct StringViewComparator
    {
        inline bool operator()(const bmcl::StringView& left, const bmcl::StringView& right) const
        {
            return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
        }
    };

    using AstMap = RcSecondMap<bmcl::StringView, Ast, StringViewComparator>;

    static PackageResult readFromFiles(Configuration* cfg, Diagnostics* diag, bmcl::ArrayView<std::string> files);
    static PackageResult decodeFromMemory(Configuration* cfg, Diagnostics* diag, const void* src, std::size_t size);

    ~Package();

    void encode(bmcl::Buffer* dest) const;

    AstMap::ConstRange modules() const;
    AstMap::Range modules();
    ComponentMap::ConstRange components() const;
    ComponentMap::Range components();
    const Diagnostics* diagnostics() const;
    Diagnostics* diagnostics();
    CompAndMsgVecConstRange statusMsgs() const;

    bmcl::OptionPtr<Ast> moduleWithName(bmcl::StringView name);
    bmcl::OptionPtr<const Ast> moduleWithName(bmcl::StringView name) const;

    void sortComponentsByNumber();

private:
    Package(Configuration* cfg, Diagnostics* diag);

    bool addFile(const char* path, Parser* p);
    void addAst(Ast* ast);
    bool resolveAll();
    bool resolveImports(Ast* ast);
    bool resolveGenerics(Ast* ast);
    bool resolveStatuses(Ast* ast);
    bool resolveParameters(Ast* ast, uint64_t* paramNum);

    bool resolveVarRegexp(Ast* ast, Component* comp, VarRegexp* regexp);

    bool mapComponent(Ast* ast);

    Rc<Diagnostics> _diag;
    Rc<Configuration> _cfg;
    AstMap _modNameToAstMap;
    ComponentMap _components;
    CompAndMsgVec _statusMsgs;
};

}
