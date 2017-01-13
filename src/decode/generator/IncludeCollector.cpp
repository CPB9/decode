#include "decode/generator/IncludeCollector.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/parser/Type.h"

namespace decode {

inline bool IncludeCollector::visitEnumType(const EnumType* enumeration)
{
    addInclude(enumeration);
    return true;
}

inline bool IncludeCollector::visitStructType(const StructType* str)
{
    addInclude(str);
    return true;
}

inline bool IncludeCollector::visitVariantType(const VariantType* variant)
{
    addInclude(variant);
    return true;
}

inline bool IncludeCollector::visitImportedType(const ImportedType* u)
{
    addInclude(u->link().get());
    return false;
}

inline bool IncludeCollector::visitAliasType(const AliasType* alias)
{
    addInclude(alias);
    return false;
}

bool IncludeCollector::visitSliceType(const SliceType* slice)
{
    SrcBuilder path;
    path.append("_slices_/");
    TypeNameGen gen(&path);
    gen.genTypeName(slice);

    _dest->insert(std::move(path.result()));
    return false;
}

void IncludeCollector::addInclude(const NamedType* type)
{
    if (_currentType == type) {
        return;
    }
    std::string path = type->moduleName().toStdString();
    path.push_back('/');
    path.append(type->name().begin(), type->name().end());
    _dest->insert(std::move(path));
}

void IncludeCollector::collectIncludesAndFwdsForType(const Type* type, std::unordered_set<std::string>* dest)
{
    _dest = dest;
    _currentType = type;
    traverseType(type);
}
}

