#pragma once

#include "decode/Config.h"
#include "decode/core/HashSet.h"

#include "bmcl/Fwd.h"

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
    void appendTestedType(const ModuleInfo* info, const Type* type);

    SrcBuilder* _output;
    HashSet<const Type*> _validatedTypes;
};
}
