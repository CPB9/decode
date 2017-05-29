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
#include "decode/core/Hash.h"
#include "decode/parser/Containers.h"

#include <bmcl/ResultFwd.h>

#include <unordered_map>
#include <map>

namespace bmcl {
class Buffer;
}

namespace decode {

class Ast;
class Diagnostics;
class Parser;
class Package;
class Component;
class StatusRegexp;
class Configuration;
struct ComponentAndMsg;

using PackageResult = bmcl::Result<Rc<Package>, void>;

class Package : public RefCountable {
public:
    struct StringViewComparator
    {
        inline bool operator()(const bmcl::StringView& left, const bmcl::StringView& right) const
        {
            return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
        }
    };

    using AstMap = RcSecondMap<bmcl::StringView, Ast, StringViewComparator>;

    static PackageResult readFromDirectories(Configuration* cfg, Diagnostics* diag, bmcl::ArrayView<std::string> dirs);
    static PackageResult readFromDirectory(Configuration* cfg, Diagnostics* diag, const char* path);
    static PackageResult decodeFromMemory(Diagnostics* diag, const void* src, std::size_t size);

    ~Package();

    bmcl::Buffer encode() const;
    bmcl::Buffer encode(unsigned compressionLevel) const;

    AstMap::ConstRange modules() const;
    AstMap::Range modules();
    ComponentMap::ConstRange components() const;
    const Diagnostics* diagnostics() const;
    Diagnostics* diagnostics();
    CompAndMsgVecConstRange statusMsgs() const;

private:
    Package(Configuration* cfg, Diagnostics* diag);
    bool addFile(const char* path, Parser* p);
    bool addDir(const char* path, Package* package, Parser* p);
    void addAst(Ast* ast);
    bool resolveAll();
    bool resolveTypes(Ast* ast);
    bool resolveStatuses(Ast* ast);
    bool mapComponent(Ast* ast);

    Rc<Diagnostics> _diag;
    Rc<Configuration> _cfg;
    AstMap _modNameToAstMap;
    ComponentMap _components;
    CompAndMsgVec _statusMsgs;
};

inline ComponentMap::ConstRange Package::components() const
{
    return _components;
}

inline Package::AstMap::ConstRange Package::modules() const
{
    return _modNameToAstMap;
}

inline Package::AstMap::Range Package::modules()
{
    return _modNameToAstMap;
}

inline const Diagnostics* Package::diagnostics() const
{
    return _diag.get();
}

inline Diagnostics* Package::diagnostics()
{
    return _diag.get();
}

inline CompAndMsgVecConstRange Package::statusMsgs() const
{
    return _statusMsgs;
}
}
