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

    const ModuleInfo* moduleInfo() const
    {
        return _moduleInfo.get();
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
    using Types = RcVec<ImportedType>;
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

protected:
    TypeImport() = default;

private:
    friend class Parser;

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

    Functions _funcs;
};

}
