#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/ResultFwd.h>

#include <unordered_map>

namespace decode {

class Ast;
class Diagnostics;

class Model : public RefCountable {
public:
    static Rc<Model> readFromDirectory(const Rc<Diagnostics>& diag, const char* path);

private:
    bool addFile(const char* path, const char* fileNameStart, std::size_t fileNameLen);
    bool resolveAll();

    Rc<Diagnostics> _diag;
    std::unordered_map<std::string, Rc<Ast>> _parsedFiles;
};
}
