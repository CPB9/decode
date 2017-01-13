#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"

#include <string>
#include <unordered_set>

namespace decode {

class Type;
class TypeReprGen;

class IncludeCollector : public ConstAstVisitor<IncludeCollector> {
public:
    void collectIncludesAndFwdsForType(const Type* type, std::unordered_set<std::string>* dest);

    bool visitEnumType(const EnumType* enumeration);
    bool visitStructType(const StructType* str);
    bool visitVariantType(const VariantType* variant);
    bool visitImportedType(const ImportedType* u);
    bool visitSliceType(const SliceType* slice);
    bool visitAliasType(const AliasType* alias);

private:

    void addInclude(const NamedType* type);

    const Type* _currentType;
    std::unordered_set<std::string>* _dest;
};

}
