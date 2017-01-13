#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Location.h"
#include "decode/core/Hash.h"
#include "decode/parser/Span.h"
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

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

protected:
    Decl() = default;

    void cloneDeclTo(Decl* dest)
    {
        dest->_startLoc = _startLoc;
        dest->_endLoc = _endLoc;
        dest->_start = _start;
        dest->_end = _end;
        dest->_moduleInfo = _moduleInfo;
    }

private:
    friend class Parser;

    Location _startLoc;
    Location _endLoc;
    const char* _start;
    const char* _end;
    Rc<ModuleInfo> _moduleInfo;
};

class NamedDecl : public Decl {
public:
    bmcl::StringView name() const
    {
        return _name;
    }

protected:
    NamedDecl() = default;

private:
    friend class Parser;

    bmcl::StringView _name;
};

class TypeDecl : public Decl {
public:

    const Rc<Type>& type() const
    {
        return _type;
    }

protected:
    TypeDecl() = default;

private:
    friend class Parser;

    Rc<Type> _type;
};

class Module : public NamedDecl {
public:

protected:
    Module() = default;

private:
    friend class Parser;
};

class ImportedType;

class TypeImport : public Decl {
public:
    bmcl::StringView path() const
    {
        return _importPath;
    }

    const std::vector<Rc<ImportedType>>& types() const
    {
        return _types;
    }

protected:
    TypeImport() = default;

private:
    friend class Parser;

    bmcl::StringView _importPath;
    std::vector<Rc<ImportedType>> _types;
};

class FunctionType;

class ImplBlock : public NamedDecl {
public:

    const std::vector<Rc<FunctionType>>& functions() const
    {
        return _funcs;
    }

protected:
    ImplBlock() = default;

private:
    friend class Parser;

    std::vector<Rc<FunctionType>> _funcs;
};

}
