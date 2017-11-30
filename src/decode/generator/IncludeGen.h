#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/HashSet.h"

namespace decode {

class Type;
class NamedType;
class DynArrayType;
class GenericInstantiationType;
class SrcBuilder;

class IncludeGen {
public:
    IncludeGen(SrcBuilder* dest);

    //FIXME: pass types by reference
    void genOnboardIncludePaths(const HashSet<Rc<const Type>>* types, const char* ext = ".h");
    void genGcIncludePaths(const HashSet<Rc<const Type>>* types, const char* ext = ".hpp");

    void genGcIncludePaths(const Type* type, const char* ext = ".hpp");

private:
    template <bool isOnboard>
    void genIncludePaths(const HashSet<Rc<const Type>>* types);
    void genNamedInclude(const NamedType* type);
    void genNamedInclude(const NamedType* type, const NamedType* origin);

    void genOnboardDynArray(const DynArrayType* type);
    void genOnboardGenericInstantiation(const GenericInstantiationType* type);
    void appendExt();

    SrcBuilder* _output;
    const char* _ext;
};

}
