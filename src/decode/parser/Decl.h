#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"
#include "decode/parser/Location.h"
#include "decode/parser/Span.h"
#include "decode/parser/ModuleInfo.h"

#include <vector>

namespace decode {
namespace parser {

class ImportedType;

class Decl : public RefCountable {
public:

protected:
    Decl() = default;

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

class TypeDecl : public NamedDecl {
public:

protected:
    TypeDecl() = default;

private:
    friend class Parser;

};

class ModuleDecl : public NamedDecl {
public:

protected:
    ModuleDecl() = default;

private:
    friend class Parser;
};

class ImportDecl : public Decl {
public:

protected:
    ImportDecl() = default;

private:
    friend class Parser;
    bmcl::StringView _importPath;
    std::vector<Rc<ImportedType>> _types;
};

class FieldDecl : public NamedDecl {
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
