#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"

namespace decode {
namespace parser {

class Type : public RefCountable {
public:

protected:
    Type() = default;

private:
    friend class Parser;
};

class ImportedType : public Type {

protected:
    ImportedType() = default;

private:
    friend class Parser;
    bmcl::StringView _importPath;
    bmcl::StringView _name;
};

}
}
