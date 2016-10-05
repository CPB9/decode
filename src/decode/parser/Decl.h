#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"
#include "decode/parser/Location.h"
#include "decode/parser/Span.h"

#include <vector>

namespace decode {
namespace parser {

class Decl : public RefCountable {
public:

private:
    Location _startLoc;
    Location _endLoc;
    Span _span;
};

class ImportDecl : public Decl {
public:
    void setImportPath(bmcl::StringView path)
    {
        _importPath = path;
    }

    void addImport(bmcl::StringView import)
    {
        _imports.push_back(import);
    }

    bmcl::StringView importPath() const;
    const std::vector<bmcl::StringView>& imports() const;

private:
    bmcl::StringView _importPath;
    std::vector<bmcl::StringView> _imports;
};

class NamedDecl : public Decl {
public:
    void setName(bmcl::StringView name)
    {
        _name = name;
    }

    bmcl::StringView name() const
    {
        return _name;
    }

private:
    bmcl::StringView _name;
};

class FieldDecl : public NamedDecl {
};

class TypeDecl : public NamedDecl {
public:
};

class TagDecl : public TypeDecl {
public:
};

class RecordDecl : public TagDecl {
public:
};

class StructDecl : public RecordDecl {
public:
};
}
}
