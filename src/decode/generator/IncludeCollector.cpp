#include "decode/generator/IncludeCollector.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/parser/Type.h"
#include "decode/parser/Component.h"

#include <bmcl/Logging.h>

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
    addInclude(u->link());
    return false;
}

inline bool IncludeCollector::visitAliasType(const AliasType* alias)
{
    if (alias == _currentType) {
        traverseType(alias->alias()); //HACK
        return false;
    }
    addInclude(alias);
    return false;
}

bool IncludeCollector::visitSliceType(const SliceType* slice)
{
    if (slice == _currentType) {
        traverseType(slice->elementType());
        return false;
    }
    SrcBuilder path;
    path.append("_slices_/");
    TypeNameGen gen(&path);
    gen.genTypeName(slice);

    _dest->insert(std::move(path.result()));
    ascendTypeOnce(slice->elementType());
    return false;
}

void IncludeCollector::addInclude(const NamedType* type)
{
    if (type == _currentType) {
        return;
    }
    std::string path = type->moduleName().toStdString();
    path.push_back('/');
    path.append(type->name().begin(), type->name().end());
    _dest->insert(std::move(path));
}

void IncludeCollector::collect(const StatusMsg* msg, std::unordered_set<std::string>* dest)
{
    _currentType = 0;
    _dest = dest;
    //FIXME: visit only first accessor in every part
    for (const StatusRegexp* part : msg->partsRange()) {
        for (const Accessor* acc : part->accessorsRange()) {
            switch (acc->accessorKind()) {
            case AccessorKind::Field: {
                auto facc = static_cast<const FieldAccessor*>(acc);
                const Type* type = facc->field()->type();
                traverseType(type);
                break;
            }
            case AccessorKind::Subscript: {
                auto sacc = static_cast<const SubscriptAccessor*>(acc);
                const Type* type = sacc->type();
                traverseType(type);
                break;
            }
            default:
                assert(false);
            }
        }
    }
}

void IncludeCollector::collect(const Type* type, std::unordered_set<std::string>* dest)
{
    _dest = dest;
    _currentType = type;
    traverseType(type);
}

void IncludeCollector::collect(const Component* comp, std::unordered_set<std::string>* dest)
{
    _dest = dest;
    _currentType = 0;
    if (!comp->hasParams()) {
        return;
    }
    for (const Field* field : comp->paramsRange()) {
        traverseType(field->type());
    }
}
}

