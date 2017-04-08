#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/Containers.h"
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
    Generator(Diagnostics* diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromPackage(const Package* ast);
private:
    bool generateTypesAndComponents(const Ast* ast);
    bool generateSlices();
    bool generateStatusMessages(const Package* package);
    bool generateCommands(const Package* package);
    bool generateTmPrivate(const Package* package);
    bool generateSerializedPackage(const Package* package);

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path, SrcBuilder* output);
    bool dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath);

    SrcBuilder _photonPath;
    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    SrcBuilder _main;
    Rc<const Ast> _currentAst;
    std::unique_ptr<HeaderGen> _hgen;
    std::unique_ptr<SourceGen> _sgen;
    Rc<TypeReprGen> _reprGen;
    RcSecondUnorderedMap<std::string, const SliceType> _slices;
};

}
