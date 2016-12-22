#include "decode/generator/IncludeCollector.h"
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
    //HACK: link can only be named type
    addInclude(static_cast<const NamedType*>(u->link().get()));
    return false;
}

void IncludeCollector::addInclude(const NamedType* type)
{
    std::string path = type->moduleName().toStdString();
    path.push_back('/');
    path.append(type->name().begin(), type->name().end());
    _dest->insert(std::move(path));
}

void IncludeCollector::collectIncludesAndFwdsForType(const Type* type, std::unordered_set<std::string>* dest)
{
    _dest = dest;
    traverseType(type);
}
}

