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

    void addType(ImportedType* type)
    {
        _types.emplace_back(type);
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
