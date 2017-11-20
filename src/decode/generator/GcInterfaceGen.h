#pragma once

#include "decode/Config.h"
#include "decode/core/HashMap.h"
#include "decode/core/Rc.h"

#include "bmcl/Fwd.h"

#include <string>

namespace decode {

class SrcBuilder;
class Package;
class Ast;
class Type;
class NamedType;
class ModuleInfo;

class GcInterfaceGen {
public:
    GcInterfaceGen(SrcBuilder* dest);
    ~GcInterfaceGen();

    void generateHeader(const Package* package);

private:
    void appendTypeValidator(const Type* type);
    void appendNamedTypeInit(const NamedType* type, bmcl::StringView name);
    void appendTestedType(const Type* type);
    static void appendTestedType(const Type* type, SrcBuilder* dest);

    SrcBuilder* _output;
    HashMap<std::string, Rc<const Type>> _validatedTypes;
};
}
