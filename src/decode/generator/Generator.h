#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"

#include <bmcl/StringView.h>

#include <unordered_set>

namespace decode {

class Ast;
class Diagnostics;
class Package;
class HeaderGen;
class SourceGen;
class Type;

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromPackage(const Rc<Package>& ast);
private:
    bool generateFromAst(const Rc<Ast>& ast, HeaderGen* hgen, SourceGen* sgen);

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path);
    bool dump(const Type* type, bmcl::StringView ext, StringBuilder* currentPath);

    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    Rc<Ast> _currentAst;
};

}
