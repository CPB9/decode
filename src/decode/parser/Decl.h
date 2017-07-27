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
#include "decode/core/Location.h"
#include "decode/core/Hash.h"
#include "decode/parser/Span.h"
#include "decode/parser/Type.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Field.h"

#include <bmcl/Option.h>
#include <bmcl/Either.h>

#include <vector>
#include <set>
#include <unordered_map>
#include <cassert>
#include <map>

namespace decode {

class ImportedType;

class Decl : public RefCountable {
public:
    Decl(const ModuleInfo* info, Location start, Location end)
        : _moduleInfo(info)
        , _start(start)
        , _end(end)
    {
    }

    const ModuleInfo* moduleInfo() const
    {
        return _moduleInfo.get();
    }

protected:
    friend class Parser;
    void cloneDeclTo(Decl* dest)
    {
        dest->_start = _start;
        dest->_end = _end;
        dest->_moduleInfo = _moduleInfo;
    }

    Decl() = default;

private:

    Rc<const ModuleInfo> _moduleInfo;
    Location _start;
    Location _end;
};

class NamedDecl : public Decl {
public:
    NamedDecl() = default;

    bmcl::StringView name() const
    {
        return _name;
    }

private:
    friend class Parser;
    bmcl::StringView _name;
};

class TypeDecl : public Decl {
public:

    const Type* type() const
    {
        return _type.get();
    }

    Type* type()
    {
        return _type.get();
    }

protected:
    TypeDecl() = default;

private:
    friend class Parser;

    Rc<Type> _type;
};

class ModuleDecl : public Decl {
public:
    ModuleDecl(const ModuleInfo* info, Location start, Location end)
        : Decl(info, start, end)
    {
    }

    bmcl::StringView moduleName() const
    {
        return moduleInfo()->moduleName();
    }
};

class ImportedType;

class TypeImport : public RefCountable {
public:
    using Types = RcVec<ImportedType>;

    TypeImport(bmcl::StringView path)
        : _importPath(path)
    {
    }

    bmcl::StringView path() const
    {
        return _importPath;
    }

    Types::ConstRange typesRange() const
    {
        return _types;
    }

    Types::Range typesRange()
    {
        return _types;
    }

    bool addType(ImportedType* type)
    {
        auto it = std::find_if(_types.begin(), _types.end(), [type](const Rc<ImportedType>& current) {
            return current->name() == type->name();
        });
        if (it != _types.end()) {
            return false;
        }
        _types.emplace_back(type);
        return true;
    }

private:
    bmcl::StringView _importPath;
    std::vector<Rc<ImportedType>> _types;
};

class FunctionType;

class Function;

class ImplBlock : public NamedDecl {
public:
    using Functions = RcVec<Function>;

    Functions::ConstIterator functionsBegin() const
    {
        return _funcs.cbegin();
    }

    Functions::ConstIterator functionsEnd() const
    {
        return _funcs.cend();
    }

    Functions::ConstRange functionsRange() const
    {
        return _funcs;
    }

    void addFunction(Function* func)
    {
        _funcs.emplace_back(func);
    }

private:
    friend class Parser;
    ImplBlock() = default;

    Functions _funcs;
};

}
