#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"

#include <bmcl/StringView.h>

#include <unordered_map>
#include <memory>

namespace decode {

class Ast;
class Diagnostics;
class Package;
class HeaderGen;
class SourceGen;
class NamedType;
class SliceType;
class TypeReprGen;

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromPackage(const Rc<Package>& ast);
private:
    bool generateTypesAndComponents(const Rc<Ast>& ast);
    bool generateSlices();
    bool generateStatusMessages(const Rc<Package>& package);
    bool generateTmPrivate(const Rc<Package>& package);

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path, SrcBuilder* output);
    bool dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);

    SrcBuilder _photonPath;
    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    SrcBuilder _main;
    Rc<Ast> _currentAst;
    std::unique_ptr<HeaderGen> _hgen;
    std::unique_ptr<SourceGen> _sgen;
    Rc<TypeReprGen> _reprGen;
    std::unordered_map<std::string, Rc<SliceType>> _slices;
};

}
